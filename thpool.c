/* ********************************
 * Author:       Johan Hanssen Seferidis
 * License:	     MIT
 * Description:  Library providing a threading pool where you can add
 *               work. For usage, check the thpool.h file or README.md
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
#if defined(__linux__)
#include <sys/prctl.h>
#endif
#include "thpool.h"

#if !defined(DISABLE_PRINT) || defined(THPOOL_DEBUG)
#define print_error(msg) fprintf(stderr, msg)
#else
#define print_error(msg)
#endif

static volatile int threads_keepalive;
static volatile int threads_on_hold;



/* ========================== STRUCTURES ============================ */


/* Binary semaphore */
typedef struct bsem {
	pthread_mutex_t mutex;
	pthread_cond_t   cond;
	int v;
} bsem;


/* Job */
typedef struct job{
	struct job*  prev;                   /* pointer to previous job   */
	void   (*function)(void* arg);       /* function pointer          */
	void*  arg;                          /* function's argument       */
} job;


/* Job pool */
typedef struct jobpool{
    job *front;                          /* pointer to front of pool  */
    pthread_mutex_t lock;                /* used for job count etc    */
} jobpool;


/* Job queue */
typedef struct jobqueue{
	pthread_mutex_t rwmutex;             /* used for queue r/w access */
	job  *front;                         /* pointer to front of queue */
	job  *rear;                          /* pointer to rear  of queue */
	bsem *has_jobs;                      /* flag as binary semaphore  */
	int   len;                           /* number of jobs in queue   */
} jobqueue;


/* Thread */
typedef struct thread{
	int       id;                        /* friendly id               */
	pthread_t pthread;                   /* pointer to actual thread  */
	struct thpool_* thpool_p;            /* access to thpool          */
} thread;


/* Threadpool */
typedef struct thpool_{
	thread**   threads;                  /* pointer to threads        */
	volatile int num_threads_alive;      /* threads currently alive   */
	volatile int num_threads_working;    /* threads currently working */
	pthread_mutex_t  thcount_lock;       /* used for thread count etc */
	pthread_cond_t  threads_all_idle;    /* signal to thpool_wait     */
	jobqueue  jobqueue;                  /* job queue                 */
    jobpool  jobpool;                    /* job pool                  */
} thpool_;





/* ========================== PROTOTYPES ============================ */


static int  thread_init(thpool_* thpool_p, struct thread** thread_p, int id);
static void* thread_do(struct thread* thread_p);
static void  thread_hold();
static void  thread_destroy(struct thread* thread_p);

static int jobpool_init(jobpool* jobpool_p, unsigned num);
static struct job *jobpool_pull(jobpool* jobpool_p);
static void jobpool_push(jobpool* jobpool_p, struct job *job_p);
static void jobpool_destroy(jobpool* jobpool_p);

static int   jobqueue_init(jobqueue* jobqueue_p);
static void  jobqueue_clear(jobqueue* jobqueue_p);
static void  jobqueue_push(jobqueue* jobqueue_p, struct job* newjob_p);
static struct job* jobqueue_pull(jobqueue* jobqueue_p);
static void  jobqueue_destroy(jobqueue* jobqueue_p);

static void  bsem_init(struct bsem *bsem_p, int value);
static void  bsem_reset(struct bsem *bsem_p);
static void  bsem_post(struct bsem *bsem_p);
static void  bsem_post_all(struct bsem *bsem_p);
static void  bsem_wait(struct bsem *bsem_p);





/* ========================== THREADPOOL ============================ */


/* Initialise thread pool */
struct thpool_* thpool_init(unsigned num_threads, unsigned num_jobs){

	threads_on_hold   = 0;
	threads_keepalive = 1;

	/* Make new thread pool */
	thpool_* thpool_p;
	thpool_p = (struct thpool_*)malloc(sizeof(struct thpool_));
	if (thpool_p == NULL){
		print_error("thpool_init(): Could not allocate memory for thread pool\n");
		return NULL;
	}
	thpool_p->num_threads_alive   = 0;
	thpool_p->num_threads_working = 0;

	/* Initialise the job queue */
	if (jobqueue_init(&thpool_p->jobqueue) == -1){
		print_error("thpool_init(): Could not allocate memory for job queue\n");
		free(thpool_p);
		return NULL;
	}

    if(jobpool_init(&thpool_p->jobpool, num_jobs) != 0){
        print_error("thpool_init(): Could not allocate memory for job pool.\n");
		jobqueue_destroy(&thpool_p->jobqueue);
        free(thpool_p);
        return NULL;
    }

	/* Make threads in pool */
	thpool_p->threads = (struct thread**)malloc(num_threads * sizeof(struct thread *));
	if (thpool_p->threads == NULL){
		print_error("thpool_init(): Could not allocate memory for threads\n");
		jobqueue_destroy(&thpool_p->jobqueue);
        jobpool_destroy(&thpool_p->jobpool);
		free(thpool_p);
		return NULL;
	}

	pthread_mutex_init(&(thpool_p->thcount_lock), NULL);
	pthread_cond_init(&thpool_p->threads_all_idle, NULL);

	/* Thread init */
	int n;
	for (n=0; n<num_threads; n++){
		thread_init(thpool_p, &thpool_p->threads[n], n);
#if defined(THPOOL_DEBUG)
			printf("THPOOL_DEBUG: Created thread %d in pool \n", n);
#endif
	}

	/* Wait for threads to initialize */
	while (thpool_p->num_threads_alive != num_threads) {}

	return thpool_p;
}


/* Add work to the thread pool */
int thpool_add_work(thpool_* thpool_p, void (*function_p)(void*), void* arg_p){
	job* newjob;

	newjob = jobpool_pull(&thpool_p->jobpool);
	if (newjob == NULL){
		print_error("thpool_add_work(): Could not allocate memory for new job\n");
		return -1;
	}

	/* add function and argument */
	newjob->function=function_p;
	newjob->arg=arg_p;

	/* add job to queue */
	jobqueue_push(&thpool_p->jobqueue, newjob);

	return 0;
}


/* Wait until all jobs have finished */
void thpool_wait(thpool_* thpool_p){
	pthread_mutex_lock(&thpool_p->thcount_lock);
	while (thpool_p->jobqueue.len || thpool_p->num_threads_working) {
		pthread_cond_wait(&thpool_p->threads_all_idle, &thpool_p->thcount_lock);
	}
	pthread_mutex_unlock(&thpool_p->thcount_lock);
}


/* Destroy the threadpool */
void thpool_destroy(thpool_* thpool_p){
	/* No need to destory if it's NULL */
	if (thpool_p == NULL) return ;

	volatile int threads_total = thpool_p->num_threads_alive;

	/* End each thread 's infinite loop */
	threads_keepalive = 0;

	/* Give one second to kill idle threads */
	double TIMEOUT = 1.0;
	time_t start, end;
	double tpassed = 0.0;
	time (&start);
	while (tpassed < TIMEOUT && thpool_p->num_threads_alive){
		bsem_post_all(thpool_p->jobqueue.has_jobs);
		time (&end);
		tpassed = difftime(end,start);
	}

	/* Poll remaining threads */
	while (thpool_p->num_threads_alive){
		bsem_post_all(thpool_p->jobqueue.has_jobs);
		sleep(1);
	}

	/* Job queue cleanup */
	jobqueue_destroy(&thpool_p->jobqueue);

    /* Job pool cleanup */
    jobpool_destroy(&thpool_p->jobpool);

	/* Deallocs */
	int n;
	for (n=0; n < threads_total; n++){
		thread_destroy(thpool_p->threads[n]);
	}
	free(thpool_p->threads);
	free(thpool_p);
}


/* Pause all threads in threadpool */
void thpool_pause(thpool_* thpool_p) {
	int n;
	for (n=0; n < thpool_p->num_threads_alive; n++){
		pthread_kill(thpool_p->threads[n]->pthread, SIGUSR1);
	}
}


/* Resume all threads in threadpool */
void thpool_resume(thpool_* thpool_p) {
	threads_on_hold = 0;
}


int thpool_num_threads_working(thpool_* thpool_p){
	return thpool_p->num_threads_working;
}


int thpool_num_jobs_pooled(thpool_* thpool_p){
    pthread_mutex_lock(&thpool_p->jobpool.lock);
    int result = 0;
    struct job *cur = thpool_p->jobpool.front;
    while(cur != NULL){
        result++;
        cur = cur->prev;
    }
    pthread_mutex_unlock(&thpool_p->jobpool.lock);
    return result;
}



/* ==========================   JOBPOOL  ============================ */

/* Initialise job pool */
static int jobpool_init(jobpool* jobpool_p, unsigned num){
    if(jobpool_p == NULL){
        print_error("jobpool_init(): Cannot pass NULL pointer as job pool.\n");
        return -1;
    }

    jobpool_p->front = NULL;
	pthread_mutex_init(&jobpool_p->lock, NULL);

    unsigned active_jobs;
    for(active_jobs = 0; active_jobs < num; active_jobs++){
        struct job *temp = malloc(sizeof *temp);
        if(temp == NULL){
            // not sure if this should fail entirely and free all jobs
            // or fail silently and move on despite the memory issues
            break;
        }
        temp->function = NULL;
        temp->arg = NULL;
        temp->prev = jobpool_p->front;
        jobpool_p->front = temp;
    }

    return 0;
}

static struct job *jobpool_pull(jobpool* jobpool_p){
    if(jobpool_p == NULL){
        print_error("jobpool_pull(): Cannot pass NULL pointer as job pool.\n");
        return NULL;
    }

    pthread_mutex_lock(&jobpool_p->lock);
    struct job *ret = jobpool_p->front;
    if(ret == NULL){
#if defined(THPOOL_DEBUG)
        printf("THPOOL_DEBUG: New job added to the pool.\n");
#endif
        if((ret = malloc(sizeof *ret)) == NULL){
            print_error("jobpool_pull(): Could not allocate memory for new job\n");
            return NULL;
        }
    } else{
        jobpool_p->front = ret->prev;
    }
    pthread_mutex_unlock(&jobpool_p->lock);

    return ret;
}

static void jobpool_push(jobpool* jobpool_p, struct job *job_p){
    if(jobpool_p == NULL || job_p == NULL){
        print_error("jobpool_push(): Cannot pass NULL pointer as job pool.\n");
        return;
    }

    pthread_mutex_lock(&jobpool_p->lock);
    job_p->prev = jobpool_p->front;
    jobpool_p->front = job_p;
    pthread_mutex_unlock(&jobpool_p->lock);
}

static void jobpool_destroy(jobpool* jobpool_p){
    if(jobpool_p == NULL){
        print_error("jobpool_destroy(): Cannot pass NULL pointer as job pool.\n");
        return;
    }

    while(jobpool_p->front != NULL){
        struct job *temp = jobpool_p->front;
        jobpool_p->front = jobpool_p->front->prev;
        free(temp);
    }
}



/* ============================ THREAD ============================== */


/* Initialize a thread in the thread pool
 *
 * @param thread        address to the pointer of the thread to be created
 * @param id            id to be given to the thread
 * @return 0 on success, -1 otherwise.
 */
static int thread_init (thpool_* thpool_p, struct thread** thread_p, int id){

	*thread_p = (struct thread*)malloc(sizeof(struct thread));
	if (thread_p == NULL){
		print_error("thread_init(): Could not allocate memory for thread\n");
		return -1;
	}

	(*thread_p)->thpool_p = thpool_p;
	(*thread_p)->id       = id;

	pthread_create(&(*thread_p)->pthread, NULL, (void *)thread_do, (*thread_p));
	pthread_detach((*thread_p)->pthread);
	return 0;
}


/* Sets the calling thread on hold */
static void thread_hold () {
	threads_on_hold = 1;
	while (threads_on_hold){
		sleep(1);
	}
}


/* What each thread is doing
*
* In principle this is an endless loop. The only time this loop gets interuppted is once
* thpool_destroy() is invoked or the program exits.
*
* @param  thread        thread that will run this function
* @return nothing
*/
static void* thread_do(struct thread* thread_p){

	/* Set thread name for profiling and debuging */
	char thread_name[128] = {0};
	sprintf(thread_name, "thread-pool-%d", thread_p->id);

#if defined(__linux__)
	/* Use prctl instead to prevent using _GNU_SOURCE flag and implicit declaration */
	prctl(PR_SET_NAME, thread_name);
#elif defined(__APPLE__) && defined(__MACH__)
	pthread_setname_np(thread_name);
#else
	print_error("thread_do(): pthread_setname_np is not supported on this system");
#endif

	/* Assure all threads have been created before starting serving */
	thpool_* thpool_p = thread_p->thpool_p;

	/* Register signal handler */
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = thread_hold;
	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		print_error("thread_do(): cannot handle SIGUSR1");
	}

	/* Mark thread as alive (initialized) */
	pthread_mutex_lock(&thpool_p->thcount_lock);
	thpool_p->num_threads_alive += 1;
	pthread_mutex_unlock(&thpool_p->thcount_lock);

	while(threads_keepalive){

		bsem_wait(thpool_p->jobqueue.has_jobs);

		if (threads_keepalive){

			pthread_mutex_lock(&thpool_p->thcount_lock);
			thpool_p->num_threads_working++;
			pthread_mutex_unlock(&thpool_p->thcount_lock);

			/* Read job from queue and execute it */
			void (*func_buff)(void*);
			void*  arg_buff;
			job* job_p = jobqueue_pull(&thpool_p->jobqueue);
			if (job_p) {
				func_buff = job_p->function;
				arg_buff  = job_p->arg;
				func_buff(arg_buff);
                jobpool_push(&thpool_p->jobpool, job_p);
			}

			pthread_mutex_lock(&thpool_p->thcount_lock);
			thpool_p->num_threads_working--;
			if (!thpool_p->num_threads_working) {
				pthread_cond_signal(&thpool_p->threads_all_idle);
			}
			pthread_mutex_unlock(&thpool_p->thcount_lock);

		}
	}
	pthread_mutex_lock(&thpool_p->thcount_lock);
	thpool_p->num_threads_alive --;
	pthread_mutex_unlock(&thpool_p->thcount_lock);

	return NULL;
}


/* Frees a thread  */
static void thread_destroy (thread* thread_p){
	free(thread_p);
}





/* ============================ JOB QUEUE =========================== */


/* Initialize queue */
static int jobqueue_init(jobqueue* jobqueue_p){
	jobqueue_p->len = 0;
	jobqueue_p->front = NULL;
	jobqueue_p->rear  = NULL;

	jobqueue_p->has_jobs = (struct bsem*)malloc(sizeof(struct bsem));
	if (jobqueue_p->has_jobs == NULL){
		return -1;
	}

	pthread_mutex_init(&(jobqueue_p->rwmutex), NULL);
	bsem_init(jobqueue_p->has_jobs, 0);

	return 0;
}


/* Clear the queue */
static void jobqueue_clear(jobqueue* jobqueue_p){

	while(jobqueue_p->len){
		free(jobqueue_pull(jobqueue_p));
	}

	jobqueue_p->front = NULL;
	jobqueue_p->rear  = NULL;
	bsem_reset(jobqueue_p->has_jobs);
	jobqueue_p->len = 0;

}


/* Add (allocated) job to queue
 */
static void jobqueue_push(jobqueue* jobqueue_p, struct job* newjob){

	pthread_mutex_lock(&jobqueue_p->rwmutex);
	newjob->prev = NULL;

	switch(jobqueue_p->len){

		case 0:  /* if no jobs in queue */
					jobqueue_p->front = newjob;
					jobqueue_p->rear  = newjob;
					break;

		default: /* if jobs in queue */
					jobqueue_p->rear->prev = newjob;
					jobqueue_p->rear = newjob;

	}
	jobqueue_p->len++;

	bsem_post(jobqueue_p->has_jobs);
	pthread_mutex_unlock(&jobqueue_p->rwmutex);
}


/* Get first job from queue(removes it from queue)
<<<<<<< HEAD
 *
 * Notice: Caller MUST hold a mutex
=======
>>>>>>> da2c0fe45e43ce0937f272c8cd2704bdc0afb490
 */
static struct job* jobqueue_pull(jobqueue* jobqueue_p){

	pthread_mutex_lock(&jobqueue_p->rwmutex);
	job* job_p = jobqueue_p->front;

	switch(jobqueue_p->len){

		case 0:  /* if no jobs in queue */
		  			break;

		case 1:  /* if one job in queue */
					jobqueue_p->front = NULL;
					jobqueue_p->rear  = NULL;
					jobqueue_p->len = 0;
					break;

		default: /* if >1 jobs in queue */
					jobqueue_p->front = job_p->prev;
					jobqueue_p->len--;
					/* more than one job in queue -> post it */
					bsem_post(jobqueue_p->has_jobs);

	}

	pthread_mutex_unlock(&jobqueue_p->rwmutex);
	return job_p;
}


/* Free all queue resources back to the system */
static void jobqueue_destroy(jobqueue* jobqueue_p){
	jobqueue_clear(jobqueue_p);
	free(jobqueue_p->has_jobs);
}





/* ======================== SYNCHRONISATION ========================= */


/* Init semaphore to 1 or 0 */
static void bsem_init(bsem *bsem_p, int value) {
	if (value < 0 || value > 1) {
		print_error("bsem_init(): Binary semaphore can take only values 1 or 0");
		exit(1);
	}
	pthread_mutex_init(&(bsem_p->mutex), NULL);
	pthread_cond_init(&(bsem_p->cond), NULL);
	bsem_p->v = value;
}


/* Reset semaphore to 0 */
static void bsem_reset(bsem *bsem_p) {
	bsem_init(bsem_p, 0);
}


/* Post to at least one thread */
static void bsem_post(bsem *bsem_p) {
	pthread_mutex_lock(&bsem_p->mutex);
	bsem_p->v = 1;
	pthread_cond_signal(&bsem_p->cond);
	pthread_mutex_unlock(&bsem_p->mutex);
}


/* Post to all threads */
static void bsem_post_all(bsem *bsem_p) {
	pthread_mutex_lock(&bsem_p->mutex);
	bsem_p->v = 1;
	pthread_cond_broadcast(&bsem_p->cond);
	pthread_mutex_unlock(&bsem_p->mutex);
}


/* Wait on semaphore until semaphore has value 0 */
static void bsem_wait(bsem* bsem_p) {
	pthread_mutex_lock(&bsem_p->mutex);
	while (bsem_p->v != 1) {
		pthread_cond_wait(&bsem_p->cond, &bsem_p->mutex);
	}
	bsem_p->v = 0;
	pthread_mutex_unlock(&bsem_p->mutex);
}
