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

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include "thpool.h"

typedef struct
{
  int id;
} workload;

void task(void *arg){
	printf("Thread #%u working on %d\n", (int)pthread_self(), (int) arg);
}

void task_via_struct(void *arg){
	workload wl = *((workload *)arg);
	printf("Thread #%u working on %d\n", (int)pthread_self(), wl.id);
}

int main(){
	
	puts("Making threadpool with 4 threads");
	threadpool thpool = thpool_init(4);

	puts("Adding 40 tasks to threadpool");
	int i;
	for (i=0; i<40; i++){
		thpool_add_work(thpool, task, (void*)(uintptr_t)i);
	};

	thpool_wait(thpool);

	puts("Adding 40 tasks to threadpool using struct");
	workload *instructions[40];
	for (i=0; i<40; i++){
		instructions[i]= malloc(sizeof(workload));
		instructions[i]->id=i;
		thpool_add_work(thpool, task_via_struct, instructions[i]);
	};

	thpool_wait(thpool);

	puts("Killing threadpool");
	thpool_destroy(thpool);

	for(i=0;i<40;i++){
		free(instructions[i]);
	}
	return 0;
}
