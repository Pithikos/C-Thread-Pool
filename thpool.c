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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h> 


#include "thpool.h"	 /* here you can also find the interface to each function */

#define POLLING_INTERVAL 1

static int threads_keepalive;
static int threads_hold_flag;



/* Initialise thread pool */
thpool_t* thpool_init(int threadsN){

	threads_hold_flag = 0;
	threads_keepalive = 1;

	if (threadsN < 0){
		threadsN = 0;
	}
	
	/* Make new thread pool */
	thpool_t* thpool;
	thpool = (thpool_t*)malloc(sizeof(thpool_t));
	if (thpool==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for thread pool\n");
		return NULL;
	}

	/* Initialise the job queue */
	if (jobqueue_init(thpool)==-1){
		fprintf(stderr, "thpool_init(): Could not allocate memory for job queue\n");
		return NULL;
	}

	/* Make threads in pool */
	thpool->threads = (thread_t*)malloc(threadsN*sizeof(thread_t));
	if (thpool->threads==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for threads\n");
		return NULL;
	}
	thpool->threadsN=threadsN;
	int n;
	for (n=0; n<threadsN; n++){
		
		thread_t* th;
		th = &(thpool->threads[n]);
		(*th).id  = n;
		args_t args;
		args.arg1 = thpool;
		args.arg2 = th;
		pthread_create(&((*th).pthread), NULL, (void *)thread_do, (args_t*)&args);
		pthread_detach((*th).pthread);
		printf("Created thread %d in pool \n", (*th).id);
	}

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
	pthread_mutex_lock(&thpool->rwmutex);
	jobqueue_push(thpool, newjob);
	pthread_mutex_unlock(&thpool->rwmutex);

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

	/* End each thread 's infinite loop */
	threads_keepalive = 0;

	int any_threads_idle(thpool_t* thpool){
		int n;
		for (n=0; n < (thpool->threadsN); n++){
			if (!thpool->threads[n].working){
				return 1;
			}
		}
		return 0;
	}

	/* Kill idle threads */
	double TIMEOUT = 1.0;
	time_t start, end;
	double tpassed;
	time (&start);
	while (any_threads_idle(thpool))
	{
		while (tpassed < TIMEOUT)
		{
			bsem_post(thpool->jobqueue->has_jobs);
			time (&end);
			tpassed = difftime(end,start);
		}
		bsem_post(thpool->jobqueue->has_jobs);
	}

	/* Job queue cleanup */
	jobqueue_destroy(thpool);
	free(thpool->jobqueue);
	
	/* Dealloc */
	free(thpool->threads);
	free(thpool);

}


void thpool_pause(thpool_t* thpool) {
	threads_hold();
}


void thpool_continue(thpool_t* thpool) {
	threads_unhold();
}





/* ====================== THREAD OPERATIONS ========================= */


static void threads_hold () {
	threads_hold_flag = 1;
	while (threads_hold_flag){
		sleep(1);
	}
}


static void threads_unhold () {
	threads_hold_flag = 0;
}


static void thread_suicide() {
	pthread_exit(NULL);
}


static void thread_kill(thread_t *th, int now) {
	if (!now && (*th).working){
		sleep(1);
	}
	pthread_kill((*th).pthread, SIGTERM);
}


static void signal_handler (int signum) {
	
	switch(signum){
	
		case SIGUSR1:
			threads_hold();
			break;
			
		case SIGUSR2:
			threads_unhold();
			break;
			
		case SIGTERM:
			thread_suicide();
			break;
	}
}



/* 
 * Init point for each thread
 * 
 * */
static void thread_do(args_t* args){

	/* Assure all threads have been created before starting serving */
	thpool_t* thpool;
	thread_t* thread;
	thpool = (*args).arg1;
	thread = (*args).arg2;
	(*thread).working = 0;
	
	/* Register signal handler */
	struct sigaction act;
	act.sa_handler = signal_handler;
	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		perror("Error: cannot handle SIGUSR1");
	}
	if (sigaction(SIGUSR2, &act, NULL) == -1) {
		perror("Error: cannot handle SIGUSR2");
	}
	if (sigaction(SIGTERM, &act, NULL) == -1) {
		perror("Error: cannot handle SIGTERM");
	}
	

	while(threads_keepalive){

		bsem_wait(thpool->jobqueue->has_jobs);
		(*thread).working = 1;

		if (threads_keepalive){

			/* Read job from queue and execute it */
			void*(*func_buff)(void* arg);
			void*  arg_buff;
			job_t* job;
			pthread_mutex_lock(&thpool->rwmutex);
			job = jobqueue_pull(thpool);
			pthread_mutex_unlock(&thpool->rwmutex);
			if (job) {
				func_buff = job->function;
				arg_buff  = job->arg;
				func_buff(arg_buff);
				free(job);
			}
			(*thread).working = 0;
		}
	}
	pthread_mutex_lock(&thpool->rwmutex);
	thpool->threadsN --;
	pthread_mutex_unlock(&thpool->rwmutex);
	printf("Thread %d exiting\n", (*thread).id);
	thread_suicide();
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

	job_t* job;
	while(thpool->jobqueue->len){
		free(jobqueue_pull(thpool));
	}
	
	thpool->jobqueue->front = NULL;
	thpool->jobqueue->rear  = NULL;
	thpool->jobqueue->has_jobs->v = 0;
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
