/* ********************************
 * Author:       Johan Hanssen Seferidis
 * Date:         12/08/2011
 * License:      MIT
 * Description:  Library providing a threading pool where you can add
 *               work. For an example on usage refer to the main file
 *               found in the same package
 *
 *//** @file thpool.h *//*
 ********************************/

/* Fast reminders:
 * 
 * tp        = threadpool 
 * thpool    = threadpool
 * thpool_t  = threadpool type
 * tp_p      = threadpool pointer
 * sem       = semaphore
 * xN        = x can be any string. N stands for amount
 * 
 * */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#include "thpool.h"      /* here you can also find the interface to each function */


static int thpool_keepalive = 1;

/* Create mutex variable */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* used to serialize queue access */


/* Initialise thread pool */
thpool_t* thpool_init(int threadsN){
	thpool_t* tp_p;
	
	if (threadsN<0) threadsN=0;
	
	/* Make new thread pool */
	tp_p=(thpool_t*)malloc(sizeof(thpool_t));                              /* MALLOC thread pool */
	if (tp_p==NULL){
		fprintf(stderr, "thpool_init(): Could not allocate memory for thread pool\n");
		return NULL;
	}
	tp_p->threads=(pthread_t*)malloc(threadsN*sizeof(pthread_t));          /* MALLOC thread IDs */
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
 * */
/* There are two scenarios here. One is everything works as it should and second if
 * the thpool is to be killed. In that manner we try to BYPASS sem_wait and end each thread. */
void thpool_thread_do(thpool_t* tp_p){

	while(thpool_keepalive){

		printf("LEN: %d jobs in queue\n", jobqueue_len(tp_p));

		/* WAITING until there is work in the queue.
		 * Notice that sem_wait will decrement the jobqueue by 1 */
		if (sem_wait(tp_p->jobqueue->has_jobs)) {
			perror("thpool_thread_do(): Waiting for semaphore");
			exit(1);
		}

		if (thpool_keepalive){

			/* Read job from queue and execute it */
			void*(*func_buff)(void* arg);
			void*  arg_buff;
			job_t* job_p;

            puts("pulling job");
			job_p = jobqueue_pull(tp_p);

			if (job_p) {
				puts("WILL RUN THE JOB NOW");
				func_buff=job_p->function;
				arg_buff =job_p->arg;
				func_buff(arg_buff);               		   /* run function */
				free(job_p);                              /* DEALLOC job */
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
	
	newJob=(job_t*)malloc(sizeof(job_t));  /* MALLOC job */
	if (newJob==NULL){
		fprintf(stderr, "thpool_add_work(): Could not allocate memory for new job\n");
		exit(1);
	}
	
	/* add function and argument */
	newJob->function=function_p;
	newJob->arg=arg_p;
	
	/* add job to queue */
	jobqueue_push(tp_p, newJob);

	printf("pushed job. queue len: %d\n", jobqueue_len(tp_p));
	
	return 0;
}


/* Destroy the threadpool */
void thpool_destroy(thpool_t* tp_p){
	int t;
	
	/* End each thread's infinite loop */
	thpool_keepalive=0; 

	/* Awake idle threads waiting at semaphore */
	for (t=0; t<(tp_p->threadsN); t++){
		if (sem_post(tp_p->jobqueue->has_jobs)){
			fprintf(stderr, "thpool_destroy(): Could not bypass sem_wait()\n");
		}
	}
	
	/* Wait for threads to finish */
	for (t=0; t<(tp_p->threadsN); t++){
		pthread_join(tp_p->threads[t], NULL);
	}
	
	jobqueue_empty(tp_p);
	
	/* Dealloc */
	free(tp_p->threads);                                                   /* DEALLOC threads             */
	free(tp_p->jobqueue);                                                  /* DEALLOC job queue           */
	free(tp_p);                                                            /* DEALLOC thread pool         */
}



/* =================== JOB QUEUE OPERATIONS ===================== */



/* Initialise queue */
int jobqueue_init(thpool_t* tp_p){
	tp_p->jobqueue=(thpool_jobqueue*)malloc(sizeof(thpool_jobqueue));      /* MALLOC */
	if (tp_p->jobqueue==NULL) return -1;
	tp_p->jobqueue->tail=NULL;
	tp_p->jobqueue->head=NULL;
	tp_p->jobqueue->has_jobs=(sem_t*)malloc(sizeof(sem_t));                /* MALLOC */               
	sem_init(tp_p->jobqueue->has_jobs, 0, 0);
	return 0;
}


/* How many jobs currently in queue */
int jobqueue_len(thpool_t* tp_p){
   return tp_p->jobqueue->len;
}


/* Add job to queue */
void jobqueue_push(thpool_t* tp_p, job_t* newjob_p){ /* remember that job prev and next point to NULL */
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
	pthread_mutex_unlock(&mutex);
}


/* Get first element from queue */
job_t* jobqueue_pull(thpool_t* tp_p){
   
   /* get first job */
   pthread_mutex_lock(&mutex);
   job_t* job_p;
   job_p = tp_p->jobqueue->head;
   printf("jobqueue_pull():   queue length: %d\n", jobqueue_len(tp_p));

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
	pthread_mutex_unlock(&mutex);
	return job_p;
}


/* Remove and deallocate all jobs in queue */
void jobqueue_empty(thpool_t* tp_p){
	
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
