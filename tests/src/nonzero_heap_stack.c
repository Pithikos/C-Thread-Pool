/*
 * Try to run thpool with a non-zero heap and stack
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../../thpool.h"


void task(){
	printf("Thread #%u working on task\n", (int)pthread_self());
}


void nonzero_stack(){
    char buf[40096];
    memset(buf, 0x80, 40096);
}


void nonzero_heap(){

    int i;
    void *ptrs[200];

    for (i=0; i<200; i++){
        ptrs[i] = malloc((i+1) << 4);
        if (ptrs[i])
            memset(ptrs[i], 0x80, (i+1) << 4);
    }
    for (i=0; i<200; i++){
        free(ptrs[i]);
    }
}


int main(){

	nonzero_stack();
	nonzero_heap();

	puts("Making threadpool with 4 threads");
	threadpool thpool = thpool_init(4);

	puts("Adding 20 tasks to threadpool");
	int i;
	for (i=0; i<20; i++){
		thpool_add_work(thpool, (void*)task, NULL);
	};

	puts("Killing threadpool");
	thpool_destroy(thpool);
	
	return 0;
}
