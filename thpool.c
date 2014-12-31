/* ********************************
 * Author:		 Johan Hanssen Seferidis
 * Date:			12/08/2011
 * License:		MIT
 * Description:  Library providing a threading pool where you can add
 *					work. For an example on usage refer to the main file
 *					found in the same package
 *
 *//** @file thpool.h *//*
 ********************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h> 


#include "thpool.h"	 /* here you can also find the interface to each function */

#define POLLING_INTERVAL 1

static int thpool_keepalive = 1;





/* Initialise thread pool */
thpool_t* thpool_init(int threadsN){

	if (threadsN < 0) {
		threadsN = 0;
	}
	
	/* Make new thread pool */
	thpool_t* thpool;
	thpool=(thpool_t*)malloc(sizeof(thpool_t));
	if (thpool==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for thread pool\n");
		return NULL;
	}
	thpool->threads=(pthread_t*)malloc(threadsN*sizeof(pthread_t));
	if (thpool->threads==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for threads\n");
		return NULL;
	}
	thpool->threadsN=threadsN;


	/* Initialise the job queue */
	if (jobqueue_init(thpool)==-1){
		fprintf(stderr, "thpool_init(): Could not allocate memory for job queue\n");
		return NULL;
	}


	/* Make threads in pool */
	int n;
	for (n=0; n<threadsN; n++){
		printf("Created thread %d in pool \n", n);
		pthread_create(&(thpool->threads[n]), NULL, (void *)thpool_thread_do, (thpool_t*)thpool);
	}
	
	return thpool;
}


/* What each individual thread is doing 
 * */
static void thpool_thread_do(thpool_t* thpool){

	while(thpool_keepalive){

		bsem_wait(thpool->jobqueue->has_jobs);

		if (thpool_keepalive){

			/* Read job from queue and execute it */
			void*(*func_buff)(void* arg);
			void*  arg_buff;
			job_t* job;
			pthread_mutex_lock(&thpool->rwmutex);
			job = jobqueue_pull(thpool);
			pthread_mutex_unlock(&thpool->rwmutex);
			if (job) {
				printf("%u: Will run pulled job\n", (int)pthread_self());
				func_buff = job->function;
				arg_buff  = job->arg;
				func_buff(arg_buff);
				free(job);
			}

		}
	}
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
	pthread_mutex_lock(&thpool->rwmutex);
	jobqueue_push(thpool, newjob);
	pthread_mutex_unlock(&thpool->rwmutex);

	return 0;
}


/* Wait until all jobs in queue have finished */
void thpool_wait(thpool_t* thpool){
	while (thpool->jobqueue->len > 0) {
		sleep(POLLING_INTERVAL);
	}
}


/* Destroy the threadpool */
void thpool_destroy(thpool_t* thpool){

	/* End each thread's infinite loop */
	thpool_keepalive = 0;

	/* Kill idle threads */
	double TIMEOUT = 1.0;
	time_t start,end;
	double tpassed;
	time (&start);
	while (tpassed < TIMEOUT){
		bsem_post(thpool->jobqueue->has_jobs);
		time (&end);
		tpassed = difftime(end,start);
	}
	
	/* Wait for working threads to finish their work*/
	int n;
	for (n=0; n < (thpool->threadsN); n++){
		pthread_join(thpool->threads[n], NULL);
	}
	
	//sleep(2);

	/* Job queue cleanup */
	pthread_mutex_lock(&thpool->rwmutex);
	jobqueue_destroy(thpool);
	pthread_mutex_unlock(&thpool->rwmutex);
	//free(thpool->jobqueue);
	
	
	/* Dealloc */
	//free(thpool->threads);
	//free(thpool);
	printf("DEALLOC finito\n");
	
}





/* ===================== JOB QUEUE OPERATIONS ======================= */


/* Initialise queue */
static int jobqueue_init(thpool_t* thpool){
	thpool->jobqueue=(jobqueue_t*)malloc(sizeof(jobqueue_t));
	if (thpool->jobqueue==NULL){
		return -1;
	}
	thpool->jobqueue->has_jobs = (bsem_t*)malloc(sizeof(bsem_t));
	jobqueue_clear(thpool);
	return 0;
}


/* Clear queue */
static void jobqueue_clear(thpool_t* thpool){
	
	job_t* curjob;
	curjob = thpool->jobqueue->front;
	
	//printf("JOBS: %d\n", thpool->jobqueue->len);
	//printf("rear: %p\n", thpool->jobqueue->rear);
	//printf("curjob prev: %p\n", thpool->jobqueue->rear->prev);
	//thpool->jobqueue->tail = curjob->prev;
	while(thpool->jobqueue->len){
	//	thpool->jobqueue->tail = curjob->prev;
	//	free(curjob);
	//	curjob=thpool->jobqueue->tail;
	//}
	
	thpool->jobqueue->front = NULL;
	thpool->jobqueue->rear = NULL;
	thpool->jobqueue->has_jobs->v = 0;
	thpool->jobqueue->len = 0;
	
}


/* Add job to queue */
static void jobqueue_push(thpool_t* thpool, job_t* newjob){ /* remember that job prev and next point to NULL */

	newjob->prev = NULL;

	switch(thpool->jobqueue->len){

		case 0:  /* if there are no jobs in queue */
					thpool->jobqueue->front = newjob;
					thpool->jobqueue->rear  = newjob;
					break;

		default: /* if there are already jobs in queue */
					thpool->jobqueue->rear->prev = newjob;
					thpool->jobqueue->rear = newjob;
					
	}
	thpool->jobqueue->len++;
	bsem_post(thpool->jobqueue->has_jobs);
}


/* Get first element from queue */
static job_t* jobqueue_pull(thpool_t* thpool){

	/* get first job */
	job_t* job;
	job = thpool->jobqueue->front;

	/* remove job from queue */
	switch(thpool->jobqueue->len){
		
		case 0:  /* if there are no jobs in queue */
					return NULL;
		
		case 1:  /* if there is only one job in queue */
					thpool->jobqueue->front = NULL;
					thpool->jobqueue->rear  = NULL;
					break;
		
		default: /* if there are more than two jobs in queue */
					thpool->jobqueue->front = job->prev;
					
	}
	thpool->jobqueue->len--;
	
	// Make sure has_jobs has right value
	if (thpool->jobqueue->len > 0) {
		bsem_post(thpool->jobqueue->has_jobs);
	}

	return job;
}


/* Remove and deallocate all jobs in queue */
static void jobqueue_destroy(thpool_t* thpool){
	
	jobqueue_clear(thpool);
	
	/* Deallocs */
	free(thpool->jobqueue->has_jobs);
}





/* ======================== SYNCHRONISATION ========================= */


/* Binary semaphore post */
static void bsem_post(bsem_t *bsem) {
	pthread_mutex_lock(&bsem->mutex);
	bsem->v = 1;
	pthread_cond_signal(&bsem->cond);
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
