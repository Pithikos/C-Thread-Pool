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
		//thpool->threads[n] = malloc(sizeof(thread_t));
		//thpool->threads[n]->thpool = thpool;
		//thpool->threads[n]->id = n;
		//pthread_create(&thpool->threads[n]->pthread, NULL, (void *)thread_do, thpool->threads[n]);
		//pthread_detach(thpool->threads[n]->pthread);
			
		
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

void thread_init (thpool_t *thpool, thread_t **thread, int id){
	
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

static void thread_hold () {
	threads_on_hold = 1;
	while (threads_on_hold){
		sleep(1);
	}
}


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


static void thread_destroy (thread_t* thread){
	//puts("Destroying thread");
	free(thread);
}



/* ============================ JOB QUEUE =========================== */


/* Initialise queue */
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


/* Clear queue */
static void jobqueue_clear(thpool_t* thpool){

	while(thpool->jobqueue->len){
		free(jobqueue_pull(thpool));
	}
	
	thpool->jobqueue->front = NULL;
	thpool->jobqueue->rear  = NULL;
	bsem_reset(thpool->jobqueue->has_jobs);
	thpool->jobqueue->len = 0;
	
}


/* Add job to queue */
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


/* Get first element from queue */
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


/* Remove and deallocate all jobs in queue */
static void jobqueue_destroy(thpool_t* thpool){
	jobqueue_clear(thpool);
	free(thpool->jobqueue->has_jobs);
}





/* ======================== SYNCHRONISATION ========================= */


/* Binary semaphore init */
static void bsem_init(bsem_t *bsem, int value) {
	if (value < 0 || value > 1) {
		printf("ERROR: bsem_init(): Binary semaphore can take only values 1 or 0");
		exit(1);
	}
	bsem->v = value;
}


/* Binary semaphore reset */
static void bsem_reset(bsem_t *bsem) {
	bsem_init(bsem, 0);
}


/* Binary semaphore post */
static void bsem_post(bsem_t *bsem) {
	pthread_mutex_lock(&bsem->mutex);
	bsem->v = 1;
	pthread_cond_signal(&bsem->cond);
	pthread_mutex_unlock(&bsem->mutex);
}


/* Binary semaphore post */
static void bsem_post_all(bsem_t *bsem) {
	pthread_mutex_lock(&bsem->mutex);
	bsem->v = 1;
	pthread_cond_broadcast(&bsem->cond);
	pthread_mutex_unlock(&bsem->mutex);
}


/* Binary semaphore wait */
static void bsem_wait(bsem_t *bsem) {
	pthread_mutex_lock(&bsem->mutex);
	while (bsem->v != 1) {
		pthread_cond_wait(&bsem->cond, &bsem->mutex);
	}
	bsem->v = 0;
	pthread_mutex_unlock(&bsem->mutex);
}
