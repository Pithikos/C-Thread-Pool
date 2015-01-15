/* ********************************
 * Author:       Johan Hanssen Seferidis
 * License:	     MIT
 * Description:  Library providing a threading pool where you can add
 *               work. For an example on usage refer to the main file
 *               found in the same package
 *
 *//** @file thpool.h *//*
 * 
 ********************************/


#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h> 


#include "thpool.h"	 /* here you can also find the interface to each function */

#define POLLING_INTERVAL 1

static volatile int threads_keepalive;
static volatile int threads_on_hold;





/* ========================== STRUCTURES ============================ */


/* Binary semaphore */
typedef struct bsem_t {
	pthread_mutex_t mutex;
	pthread_cond_t   cond;
	int v;
} bsem_t;


/* Job */
typedef struct job_t{
	void*  (*function)(void* arg);       /* function pointer          */
	void*          arg;                  /* function's argument       */
	struct job_t*  prev;                 /* pointer to previous job   */
} job_t;


/* Job queue */
typedef struct jobqueue_t{
	pthread_mutex_t  rwmutex;            /* used for queue r/w access */
	job_t           *front;              /* pointer to front of queue */
	job_t           *rear;               /* pointer to rear  of queue */
	bsem_t          *has_jobs;           /* flag as binary semaphore  */
	int              len;                /* number of jobs in queue   */
} jobqueue_t;


/* Thread */
typedef struct thread_t{
	int       id;                       /* friendly id                */
	pthread_t pthread;                  /* pointer to actual thread   */
	struct thpool_t* thpool;            /* access to thpool           */
} thread_t;

/* Threadpool */
typedef struct thpool_t{
	thread_t**       threads;            /* pointer to threads        */
	int              threads_alive;      /* threads currently alive   */
	pthread_mutex_t  thcount_lock;       /* used for thread count etc */
	jobqueue_t*      jobqueue;           /* pointer to the job queue  */    
} thpool_t;





/* ========================== PROTOTYPES ============================ */

static void thread_init(thpool_t* thpool, thread_t** thread, int id);
static void* thread_do(thread_t* thread);
static void thread_hold();
static void thread_destroy(thread_t* thread);

static int jobqueue_init(thpool_t* thpool);
static void jobqueue_clear(thpool_t* thpool);
static void jobqueue_push(thpool_t* thpool, job_t* newjob);
static job_t* jobqueue_pull(thpool_t* thpool);
static void jobqueue_destroy(thpool_t* thpool);

static void bsem_init(bsem_t *bsem, int value);
static void bsem_reset(bsem_t *bsem);
static void bsem_post(bsem_t *bsem);
static void bsem_post_all(bsem_t *bsem);
static void bsem_wait(bsem_t *bsem);





/* ========================== THREADPOOL ============================ */

/* Initialise thread pool */
thpool_t* thpool_init(int threadsN){

	threads_on_hold   = 0;
	threads_keepalive = 1;

	if ( threadsN < 0){
		threadsN = 0;
	}


	/* Make new thread pool */
	thpool_t* thpool;
	thpool = (thpool_t*)malloc(sizeof(thpool_t));
	if (thpool==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for thread pool\n");
		exit(1);
	}
	thpool->threads_alive = 0;

	/* Initialise the job queue */
	if (jobqueue_init(thpool)==-1){
		fprintf(stderr, "thpool_init(): Could not allocate memory for job queue\n");
		exit(1);
	}

	/* Make threads in pool */
	thpool->threads = (thread_t**)malloc(threadsN * sizeof(thread_t));
	if (thpool->threads==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for threads\n");
		exit(1);
	}
	
	/* Thread init */
	int n;
	for (n=0; n<threadsN; n++){
		thread_init(thpool, &thpool->threads[n], n);
		printf("Created thread %d in pool \n", n);
	}
	
	/* Wait for threads to initialize */
	while (thpool->threads_alive != threadsN) {}

	return thpool;
}


/* Add work to the thread pool */
int thpool_add_work(thpool_t* thpool, void *(*function_p)(void*), void* arg_p){
	job_t* newjob;

	newjob=(job_t*)malloc(sizeof(job_t));
	if (newjob==NULL){
		fprintf(stderr, "thpool_add_work(): Could not allocate memory for new job\n");
		return -1;
	}

	/* add function and argument */
	newjob->function=function_p;
	newjob->arg=arg_p;

	/* add job to queue */
	pthread_mutex_lock(&thpool->jobqueue->rwmutex);
	jobqueue_push(thpool, newjob);
	pthread_mutex_unlock(&thpool->jobqueue->rwmutex);

	return 0;
}


/* Wait until all jobs in queue have finished */
void thpool_wait(thpool_t* thpool){
	while (thpool->jobqueue->len) {
		sleep(POLLING_INTERVAL);
	}
}


/* Destroy the threadpool */
void thpool_destroy(thpool_t* thpool){
	
	volatile int threads_total = thpool->threads_alive;

	/* End each thread 's infinite loop */
	threads_keepalive = 0;
	
	/* Give one second to kill idle threads */
	double TIMEOUT = 1.0;
	time_t start, end;
	double tpassed;
	time (&start);
	while (tpassed < TIMEOUT && thpool->threads_alive){
		bsem_post_all(thpool->jobqueue->has_jobs);
		time (&end);
		tpassed = difftime(end,start);
	}
	
	/* Poll remaining threads */
	while (thpool->threads_alive){
		bsem_post_all(thpool->jobqueue->has_jobs);
		sleep(1);
	}

	/* Job queue cleanup */
	jobqueue_destroy(thpool);
	free(thpool->jobqueue);
	
	/* Deallocs */
	int n;
	for (n=0; n < threads_total; n++){
		puts("FREEING THREAD");
		//thread_destroy(thpool->threads[n]);
		free(thpool->threads[n]);
	}
	free(thpool->threads);
	free(thpool);
}


void thpool_pause(thpool_t* thpool) {
	int n;
	for (n=0; n < thpool->threads_alive; n++){
		pthread_kill(thpool->threads[n]->pthread, SIGUSR1);
	}
}


void thpool_resume(thpool_t* thpool) {
	threads_on_hold = 0;
}





/* ============================ THREAD ============================== */


/**
 * @brief Initialize a thread in the thread pool
 * 
 * Will initialize a new thread for the given threadpool and give the
 * the thread an ID
 * 
 * Notice also that the thread's id is not populated automatically.
 * 
 * @param thread        address to the pointer of the thread to be created
 * @param id            id to be given to the thread
 * 
 */
static void thread_init (thpool_t *thpool, thread_t **thread, int id){
	
	*thread = (thread_t*)malloc(sizeof(thread_t));
	if (thread == NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for thread\n");
		exit(1);
	}

	(*thread)->thpool = thpool;
	(*thread)->id     = id;

	pthread_create(&(*thread)->pthread, NULL, (void *)thread_do, (*thread));
	pthread_detach((*thread)->pthread);
	
}


/**
 * @brief Sets the calling thread on hold until threads_on_hold is set to 1
 * @param thread
 */
static void thread_hold () {
	threads_on_hold = 1;
	while (threads_on_hold){
		sleep(1);
	}
}


/**
* @brief What each thread is doing
* 
* In principle this is an endless loop. The only time this loop gets interuppted is once
* thpool_destroy() is invoked or the program exits.
* 
* @param  thread        thread that will run this function
* @return nothing
*/
static void* thread_do(thread_t* thread){

	/* Assure all threads have been created before starting serving */
	thpool_t* thpool = thread->thpool;
	
	/* Register signal handler */
	struct sigaction act;
	act.sa_handler = thread_hold;
	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		perror("Error: cannot handle SIGUSR1");
	}
	
	/* Mark thread as alive (initialized) */
	pthread_mutex_lock(&thpool->thcount_lock);
	thpool->threads_alive += 1;
	pthread_mutex_unlock(&thpool->thcount_lock);

	while(threads_keepalive){

		bsem_wait(thpool->jobqueue->has_jobs);

		if (threads_keepalive){

			/* Read job from queue and execute it */
			void*(*func_buff)(void* arg);
			void*  arg_buff;
			job_t* job;
			pthread_mutex_lock(&thpool->jobqueue->rwmutex);
			job = jobqueue_pull(thpool);
			pthread_mutex_unlock(&thpool->jobqueue->rwmutex);
			if (job) {
				func_buff = job->function;
				arg_buff  = job->arg;
				func_buff(arg_buff);
				free(job);
			}

		}
	}
	pthread_mutex_lock(&thpool->thcount_lock);
	thpool->threads_alive --;
	pthread_mutex_unlock(&thpool->thcount_lock);

	return NULL;
}


/**
 * @brief Frees a thread
 * @param  thread
 */
static void thread_destroy (thread_t* thread){
	//puts("Destroying thread");
	free(thread);
}



/* ============================ JOB QUEUE =========================== */


/**
 * @brief  Initialize queue
 * @return 0 on success,
 *        -1 on memory allocation error
 */
static int jobqueue_init(thpool_t* thpool){
	thpool->jobqueue=(jobqueue_t*)malloc(sizeof(jobqueue_t));
	if (thpool->jobqueue==NULL){
		return -1;
	}
	thpool->jobqueue->has_jobs = (bsem_t*)malloc(sizeof(bsem_t));
	bsem_init(thpool->jobqueue->has_jobs, 0);
	jobqueue_clear(thpool);
	return 0;
}


/**
 * @brief  Clears the queue
 */
static void jobqueue_clear(thpool_t* thpool){

	while(thpool->jobqueue->len){
		free(jobqueue_pull(thpool));
	}
	
	thpool->jobqueue->front = NULL;
	thpool->jobqueue->rear  = NULL;
	bsem_reset(thpool->jobqueue->has_jobs);
	thpool->jobqueue->len = 0;
	
}


/**
 * @brief Add job to queue
 * 
 * A new job will be added to the queue. The new job MUST be allocated
 * before passed to this function or else other functions like jobqueue_empty()
 * will be broken.
 * 
 * CALLER MUST HOLD A MUTEX
 * 
 * @param pointer to the new (allocated) job
 */
static void jobqueue_push(thpool_t* thpool, job_t* newjob){ /* remember that job prev and next point to NULL */

	newjob->prev = NULL;

	switch(thpool->jobqueue->len){

		case 0:  /* if no jobs in queue */
					thpool->jobqueue->front = newjob;
					thpool->jobqueue->rear  = newjob;
					break;

		default: /* if jobs in queue */
					thpool->jobqueue->rear->prev = newjob;
					thpool->jobqueue->rear = newjob;
					
	}
	thpool->jobqueue->len++;
	bsem_post(thpool->jobqueue->has_jobs);
}


/**
 * @brief Get first job from queue(removes it from queue)
 * 
 * This does not free allocated memory so be sure to have peeked() \n
 * before invoking this as else there will result lost memory pointers.
 * 
 * CALLER MUST HOLD A MUTEX
 * 
 * @return point to job on success,
 *         NULL if there is no job in queue
 */
static job_t* jobqueue_pull(thpool_t* thpool){

	job_t* job;
	job = thpool->jobqueue->front;

	switch(thpool->jobqueue->len){
		
		case 0:  /* if no jobs in queue */
					return NULL;
		
		case 1:  /* if one job in queue */
					thpool->jobqueue->front = NULL;
					thpool->jobqueue->rear  = NULL;
					break;
		
		default: /* if >1 jobs in queue */
					thpool->jobqueue->front = job->prev;
					
	}
	thpool->jobqueue->len--;
	
	/* Make sure has_jobs has right value */
	if (thpool->jobqueue->len > 0) {
		bsem_post(thpool->jobqueue->has_jobs);
	}

	return job;
}


/**
 * @brief Remove and deallocate all jobs in queue
 * 
 * This function will deallocate all jobs in the queue and set the
 * jobqueue to its initialization values, thus tail and head pointing
 * to NULL and amount of jobs equal to 0.
 * 
 * */
static void jobqueue_destroy(thpool_t* thpool){
	jobqueue_clear(thpool);
	free(thpool->jobqueue->has_jobs);
}





/* ======================== SYNCHRONISATION ========================= */


/**
 * @brief Inits semaphore to given value (1 or 0)
 * */
static void bsem_init(bsem_t *bsem, int value) {
	if (value < 0 || value > 1) {
		printf("ERROR: bsem_init(): Binary semaphore can take only values 1 or 0");
		exit(1);
	}
	bsem->v = value;
}


/**
 * @brief Resets semaphore to 0
 * */
static void bsem_reset(bsem_t *bsem) {
	bsem_init(bsem, 0);
}


/**
 * @brief Sets semaphore to one and notifies at least one thread
 * */
static void bsem_post(bsem_t *bsem) {
	pthread_mutex_lock(&bsem->mutex);
	bsem->v = 1;
	pthread_cond_signal(&bsem->cond);
	pthread_mutex_unlock(&bsem->mutex);
}


/**
 * @brief Sets semaphore to one and notifies all threads
 * */
static void bsem_post_all(bsem_t *bsem) {
	pthread_mutex_lock(&bsem->mutex);
	bsem->v = 1;
	pthread_cond_broadcast(&bsem->cond);
	pthread_mutex_unlock(&bsem->mutex);
}


/**
 * @brief Waits on semaphore until semaphore has value 0
 * */
static void bsem_wait(bsem_t *bsem) {
	pthread_mutex_lock(&bsem->mutex);
	while (bsem->v != 1) {
		pthread_cond_wait(&bsem->cond, &bsem->mutex);
	}
	bsem->v = 0;
	pthread_mutex_unlock(&bsem->mutex);
}
