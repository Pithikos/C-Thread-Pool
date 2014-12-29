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

/* Create mutex variable */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* used to serialize queue access */


/* Initialise thread pool */
thpool_t* thpool_init(int threadsN){
	thpool_t* tp_p;
	
	if (threadsN<0) threadsN=0;
	
	/* Make new thread pool */
	tp_p=(thpool_t*)malloc(sizeof(thpool_t));
	if (tp_p==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for thread pool\n");
		return NULL;
	}
	tp_p->threads=(pthread_t*)malloc(threadsN*sizeof(pthread_t));
	if (tp_p->threads==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for thread IDs\n");
		return NULL;
	}
	tp_p->threadsN=threadsN;
	
	/* Initialise the job queue */
	if (jobqueue_init(tp_p)==-1){
		fprintf(stderr, "thpool_init(): Could not allocate memory for job queue\n");
		return NULL;
	}
	
	/* Make threads in pool */
	int t;
	for (t=0; t<threadsN; t++){
		printf("Created thread %d in pool \n", t);
		pthread_create(&(tp_p->threads[t]), NULL, (void *)thpool_thread_do, (void *)tp_p);
	}
	
	return tp_p;
}


/* What each individual thread is doing 
 * 
 * There are two scenarios here. One is everything works as it should and second if
 * the thpool is to be killed. In that manner we try to BYPASS sem_wait and end each thread. */
static void thpool_thread_do(thpool_t* tp_p){

	while(thpool_keepalive){

		bsem_wait(tp_p->jobqueue->has_jobs);

		if (thpool_keepalive){

			/* Read job from queue and execute it */
			void*(*func_buff)(void* arg);
			void*  arg_buff;
			job_t* job_p;
			job_p = jobqueue_pull(tp_p);
			if (job_p) {
				func_buff=job_p->function;
				arg_buff =job_p->arg;
				func_buff(arg_buff);
				free(job_p);
			}
			
		}
		else
		{
			return; /* EXIT thread*/
		}
	}
	return;
}


/* Add work to the thread pool */
int thpool_add_work(thpool_t* tp_p, void *(*function_p)(void*), void* arg_p){
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
	jobqueue_push(tp_p, newJob);

	return 0;
}


/* Wait until all jobs in queue have finished */
void thpool_wait(thpool_t* tp_p){
	while (tp_p->jobqueue->len > 0) {
		sleep(POLLING_INTERVAL);
	}
}


/* Destroy the threadpool */
void thpool_destroy(thpool_t* tp_p){
	int t;
	
	/* End each thread's infinite loop */
	thpool_keepalive=0; 

	/* Awake idle threads waiting at semaphore */
	
	//for (t=0; t<(tp_p->threadsN); t++){
		//bsem_post(tp_p->jobqueue->has_jobs);
		/*if (){
			fprintf(stderr, "thpool_destroy(): Could not bypass sem_wait()\n");
		}*/
	//}
	
	/* Wait for threads to finish */
	for (t=0; t < (tp_p->threadsN); t++){
		pthread_join(tp_p->threads[t], NULL);
	}

	jobqueue_empty(tp_p);

	/* Dealloc */
	free(tp_p->threads);
	free(tp_p->jobqueue);
	free(tp_p);	
}



/* ===================== JOB QUEUE OPERATIONS ======================= */

/*
 *  Requirements for job queue:
 *	 - Thread-safe push/pull
 *	 - Binary semaphore for has_jobs must be 1 if there are jobs, 0 if not
 * 
 * */


/* Initialise queue */
static int jobqueue_init(thpool_t* tp_p){
	tp_p->jobqueue=(thpool_jobqueue*)malloc(sizeof(thpool_jobqueue));
	if (tp_p->jobqueue==NULL) return -1;
	tp_p->jobqueue->tail=NULL;
	tp_p->jobqueue->head=NULL;
	tp_p->jobqueue->has_jobs=(bsem_t*)malloc(sizeof(bsem_t));			
	tp_p->jobqueue->len = 0;
	return 0;
}


/* How many jobs currently in queue */
static int jobqueue_len(thpool_t* tp_p){
	return tp_p->jobqueue->len;
}


/* Add job to queue */
static void jobqueue_push(thpool_t* tp_p, job_t* newjob_p){ /* remember that job prev and next point to NULL */

	pthread_mutex_lock(&mutex);
	newjob_p->next=NULL;

	switch(jobqueue_len(tp_p)){

		case 0:  /* if there are no jobs in queue */
					tp_p->jobqueue->tail=newjob_p;
					tp_p->jobqueue->head=newjob_p;
					break;

		default: /* if there are already jobs in queue */
					tp_p->jobqueue->tail->next=newjob_p;
					newjob_p->prev=tp_p->jobqueue->tail;
					tp_p->jobqueue->tail=newjob_p;
	}
	tp_p->jobqueue->len++;
	bsem_post(tp_p->jobqueue->has_jobs);
	pthread_mutex_unlock(&mutex);
}


/* Get first element from queue */
static job_t* jobqueue_pull(thpool_t* tp_p){
	
	pthread_mutex_lock(&mutex);
	//puts("**** PULLING");
	
	/* get first job */
	job_t* job_p;
	job_p = tp_p->jobqueue->head;
	printf("     queue length: %d\n", jobqueue_len(tp_p));
	printf("     has jobs:     %d\n", tp_p->jobqueue->has_jobs->v);

	/* remove job from queue */
	switch(jobqueue_len(tp_p)){
		
		case 0:  /* if there are no jobs in queue */
					return NULL;
					break;
		
		case 1:  /* if there is only one job in queue */
					tp_p->jobqueue->tail=NULL;
					tp_p->jobqueue->head=NULL;
					break;
					
		default: /* if there are more than one jobs in queue */
					tp_p->jobqueue->head=job_p->next;
					job_p->next->prev=tp_p->jobqueue->head;
	}
	tp_p->jobqueue->len--;
	
	// Make sure has_jobs has right value
	if (tp_p->jobqueue->len > 0) {
		bsem_post(tp_p->jobqueue->has_jobs);
	}
	puts("     (after pull)");
	printf("     queue length: %d\n", jobqueue_len(tp_p));
	printf("     has jobs:     %d\n", tp_p->jobqueue->has_jobs->v);
	
	pthread_mutex_unlock(&mutex);
	return job_p;
}


/* Remove and deallocate all jobs in queue */
static void jobqueue_empty(thpool_t* tp_p){
	
	job_t* curjob;
	curjob=tp_p->jobqueue->tail;
	
	while(jobqueue_len(tp_p)){
		tp_p->jobqueue->tail=curjob->prev;
		free(curjob);
		curjob=tp_p->jobqueue->tail;
	}
	
	/* Fix head and tail */
	tp_p->jobqueue->tail=NULL;
	tp_p->jobqueue->head=NULL;
	
	/* Deallocs */
	free(tp_p->jobqueue->has_jobs);
}



/* ======================== SYNCHRONISATION ========================= */


static void bsem_post(bsem_t *bsem) {
	 pthread_mutex_lock(&bsem->mutex);
	 bsem->v = 1;
	 pthread_cond_signal(&bsem->cond);
	 pthread_mutex_unlock(&bsem->mutex);
}


static void bsem_wait(bsem_t *bsem) {
	 pthread_mutex_lock(&bsem->mutex);
	 while (!bsem->v)
		  pthread_cond_wait(&bsem->cond, &bsem->mutex);
	 bsem->v = 0;
	 pthread_mutex_unlock(&bsem->mutex);
}
