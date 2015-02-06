#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "../../thpool.h"


/*
 * This program takes 3 arguments: number of jobs to add,
 *                                 number of threads,
 *                                 wait for each thread separetely (1)?
 *                                 how long each thread should run
 * 
 * Each job is to simply sleep for given amount of seconds.
 * 
 * */


void sleep_1(int* secs) {
	sleep(*secs);
}


int main(int argc, char *argv[]){

	char* p;
	if (argc < 3){
		puts("This testfile needs at least two arguments");
		exit(1);
	}


	int num_jobs         = strtol(argv[1], &p, 10);
	int num_threads      = strtol(argv[2], &p, 10);
	int wait_each_job    = argv[3] ? strtol(argv[3], &p, 10) : 0;
	int sleep_per_thread = argv[4] ? strtol(argv[4], &p, 10) : 1;

	threadpool thpool = thpool_init(num_threads);

	int n;
	for (n=0; n<num_jobs; n++){
		thpool_add_work(thpool, (void*)sleep_1, &sleep_per_thread);
		if (wait_each_job)
			thpool_wait(thpool);
	}
	if (!wait_each_job)
		thpool_wait(thpool);

	return 0;
}
