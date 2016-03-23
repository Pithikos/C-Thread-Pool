/* 
 * WHAT THIS EXAMPLE DOES
 * 
 * We create a pool of 4 threads and then add 40 tasks to the pool(20 task1 
 * functions and 20 task2 functions). task1 and task2 simply print which thread is running them.
 * 
 * As soon as we add the tasks to the pool, the threads will run them. It can happen that 
 * you see a single thread running all the tasks (highly unlikely). It is up the OS to
 * decide which thread will run what. So it is not an error of the thread pool but rather
 * a decision of the OS.
 * 
 * */
#include "thpool.h"

#include <stdio.h>
#include <pthread.h>
#include <stdint.h> // intptr_t


void worker(int job){
	printf("Thread #%x working on task %d\n", ((uint16_t)pthread_self()) % 0x1000, job);
	sleep(1);
	printf("Thread #%x done with task %d\n", ((uint16_t)pthread_self()) % 0xf84, job);
}


int main(){
	
	puts("Making threadpool with 4 threads");
	threadpool thpool = thpool_init((void*)worker,4);

	puts("Adding 40 tasks to threadpool");
	int i;
	for (i=0; i<20; i++){
		thpool_add_work(thpool, (void*)((intptr_t)i<<1));
		thpool_add_work(thpool, (void*)((intptr_t)(i<<1+1)));
	};

	thpool_wait(thpool);
	puts("Killing threadpool");
	thpool_destroy(thpool);
	
	return 0;
}
