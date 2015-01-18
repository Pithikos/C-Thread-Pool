#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "../../thpool.h"


/*
 * This program takes 3 arguments: number of jobs to add,
 *                                 number of threads,
 *                                 wait for each thread (1) or all threads at once (0)?
 * 
 * Each job is to simply sleep for one second.
 * 
 * */


void sleep_1() {
	sleep(1);
}


int main(int argc, char *argv[]){
	
	char* p;
	if (argc != 4){
		puts("This testfile needs excactly three arguments");
		exit(1);
	}
	
	
	int num_jobs      = strtol(argv[1], &p, 10);
	int num_threads   = strtol(argv[2], &p, 10);
	int wait_each_job = strtol(argv[3], &p, 10);

	threadpool thpool = thpool_init(num_threads);
	
	int n;
	for (n=0; n<num_jobs; n++){
		thpool_add_work(thpool, (void*)sleep_1, NULL);
		if (wait_each_job)
			thpool_wait(thpool);
	}
	if (!wait_each_job)
		thpool_wait(thpool);

	return 0;
}
