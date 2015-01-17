/* 
 * WHAT THIS EXAMPLE DOES
 * 
 * We create a pool of 4 threads and then add 40 tasks to the pool(20 task1 
 * functions and 20 task2 functions).
 * 
 * task1 and task2 simply print which thread is running them.
 * 
 * As soon as we add the tasks to the pool, the threads will run them. One thread
 * may run x tasks in a row so if you see as output the same thread running several
 * tasks, it's not an error.
 * 
 * */

#include <stdio.h>
#include "thpool.h"


void task1(){
	printf("Thread #%u working on task1\n", (int)pthread_self());
}


void task2(){
	printf("Thread #%u working on task2\n", (int)pthread_self());
}


int main(){
	
	puts("Making threadpool with 4 threads");
	threadpool thpool = thpool_init(4);

	puts("Adding 40 tasks to threadpool");
	int i;
	for (i=0; i<20; i++){
		thpool_add_work(thpool, (void*)task1, NULL);
		thpool_add_work(thpool, (void*)task2, NULL);
	};

    puts("Killing threadpool");
	thpool_destroy(thpool);
	
	return 0;
}
