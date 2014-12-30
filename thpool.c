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


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>


#include "thpool.h"	 /* here you can also find the interface to each function */

#define POLLING_INTERVAL 1

static int thpool_keepalive = 1;





/* Initialise thread pool */
thpool_t* thpool_init(int threadsN){
	thpool_t* thpool;
	
	if (threadsN<0) threadsN=0;
	
	/* Make new thread pool */
	thpool=(thpool_t*)malloc(sizeof(thpool_t));
	if (thpool==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for thread pool\n");
		return NULL;
	}
	thpool->threads=(pthread_t*)malloc(threadsN*sizeof(pthread_t));
	if (thpool->threads==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for thread IDs\n");
		return NULL;
	}
	thpool->threadsN=threadsN;
	
	/* Initialise the job queue */
	if (jobqueue_init(thpool)==-1){
		fprintf(stderr, "thpool_init(): Could not allocate memory for job queue\n");
		return NULL;
	}
	
	/* Make threads in pool */
	int t;
	for (t=0; t<threadsN; t++){
		printf("Created thread %d in pool \n", t);
		pthread_create(&(thpool->threads[t]), NULL, (void *)thpool_thread_do, (void *)thpool);
	}
	
	return thpool;
}


/* What each individual thread is doing 
 * 
 * There are two scenarios here. One is everything works as it should and second if
 * the thpool is to be killed. In that manner we try to BYPASS sem_wait and end each thread. */
static void thpool_thread_do(thpool_t* thpool){

	while(thpool_keepalive){

		//printf("**** pre bsem_wait:  bsem: %d, len: %d\n", thpool->jobqueue->has_jobs->v, thpool->jobqueue->len);
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
				func_buff=job->function;
				arg_buff =job->arg;
				func_buff(arg_buff);
				free(job);
			}
			
		}
		else
		{
			exit(0); /* EXIT thread*/
		}
	}
	exit(0);
}


/* Add work to the thread pool */
int thpool_add_work(thpool_t* thpool, void *(*function_p)(void*), void* arg_p){
	job_t* newJob;
	
	newJob=(job_t*)malloc(sizeof(job_t));
	if (newJob==NULL){
		fprintf(stderr, "thpool_add_work(): Could not allocate memory for new job\n");
		exit(1);
	}
	
	/* add function and argument */
	newJob->function=function_p;
	newJob->arg=arg_p;
	
	/* add job to queue */
	pthread_mutex_lock(&thpool->rwmutex);
	jobqueue_push(thpool, newJob);
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

	int t;

	/* End each thread's infinite loop */
	thpool_keepalive = 0;
	
	jobqueue_empty(thpool);

	//for (t=0; t < (thpool->threadsN); t++){
	//	sleep(1);
	///	bsem_post(thpool->jobqueue->has_jobs);
	//	pthread_join(thpool->threads[t], NULL);
	//}

	/* Wait for threads to finish */
	//for (t=0; t < (thpool->threadsN); t++){
	//	pthread_join(thpool->threads[t], NULL);
	//}

	/* Awake idle threads waiting at semaphore */

	//bsem_post(thpool->jobqueue->has_jobs);

	//jobqueue_empty(thpool);

	/* Dealloc */
	//free(thpool->threads);
	//free(thpool->jobqueue);
	//free(thpool);	
}





/* ===================== JOB QUEUE OPERATIONS ======================= */


/* Initialise queue */
static int jobqueue_init(thpool_t* thpool){
	thpool->jobqueue=(jobqueue_t*)malloc(sizeof(jobqueue_t));
	if (thpool->jobqueue==NULL) return -1;
	thpool->jobqueue->tail=NULL;
	thpool->jobqueue->head=NULL;
	
	thpool->jobqueue->has_jobs = (bsem_t*)malloc(sizeof(bsem_t));
	thpool->jobqueue->has_jobs->v = 0;
	
	thpool->jobqueue->len = 0;
	return 0;
}


/* Add job to queue */
static void jobqueue_push(thpool_t* thpool, job_t* newjob){ /* remember that job prev and next point to NULL */

	newjob->next=NULL;

	switch(thpool->jobqueue->len){

		case 0:  /* if there are no jobs in queue */
					thpool->jobqueue->tail=newjob;
					thpool->jobqueue->head=newjob;
					break;

		default: /* if there are already jobs in queue */
					thpool->jobqueue->tail->next=newjob;
					newjob->prev=thpool->jobqueue->tail;
					thpool->jobqueue->tail=newjob;
	}
	thpool->jobqueue->len++;
	bsem_post(thpool->jobqueue->has_jobs);
}


/* Get first element from queue */
static job_t* jobqueue_pull(thpool_t* thpool){
	
	
	
	/* get first job */
	job_t* job;
	job = thpool->jobqueue->head;

	/* remove job from queue */
	switch(thpool->jobqueue->len){
		
		case 0:  /* if there are no jobs in queue */
					return NULL;
					break;
		
		case 1:  /* if there is only one job in queue */
					thpool->jobqueue->tail=NULL;
					thpool->jobqueue->head=NULL;
					break;
					
		default: /* if there are more than one jobs in queue */
					thpool->jobqueue->head=job->next;
					job->next->prev=thpool->jobqueue->head;
	}
	thpool->jobqueue->len--;
	
	// Make sure has_jobs has right value
	if (thpool->jobqueue->len > 0) {
		bsem_post(thpool->jobqueue->has_jobs);
	}

	return job;
}


/* Remove and deallocate all jobs in queue */
static void jobqueue_empty(thpool_t* thpool){

	job_t* curjob;
	curjob=thpool->jobqueue->tail;
	
	while(thpool->jobqueue->len){
		thpool->jobqueue->tail=curjob->prev;
		free(curjob);
		curjob=thpool->jobqueue->tail;
	}
	
	/* Fix head and tail */
	thpool->jobqueue->tail=NULL;
	thpool->jobqueue->head=NULL;
	
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
