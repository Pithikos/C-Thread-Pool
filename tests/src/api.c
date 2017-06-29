#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "../../thpool.h"


void sleep_2_secs(){
	sleep(2);
	puts("SLEPT");
}


int main(int argc, char *argv[]){

	int num = 0;
	threadpool thpool;

	/* Test if we can get the current number of working threads */
	thpool = thpool_init(10);
	thpool_add_work(thpool, (void*)sleep_2_secs, NULL);
	thpool_add_work(thpool, (void*)sleep_2_secs, NULL);
	thpool_add_work(thpool, (void*)sleep_2_secs, NULL);
	thpool_add_work(thpool, (void*)sleep_2_secs, NULL);
	sleep(1);
	num = thpool_num_threads_working(thpool);
	if (thpool_num_threads_working(thpool) != 4) {
		printf("Expected 4 threads working, got %d", num);
		return -1;
	};

	/* Test (same as above) */
	thpool = thpool_init(5);
	thpool_add_work(thpool, (void*)sleep_2_secs, NULL);
	thpool_add_work(thpool, (void*)sleep_2_secs, NULL);
	sleep(1);
	num = thpool_num_threads_working(thpool);
	if (num != 2) {
		printf("Expected 2 threads working, got %d", num);
		return -1;
	};

	/* Test jobs placed and jobs done counters */
	long count;
	thpool = thpool_init(2);
	thpool_add_work(thpool, (void*)sleep_2_secs, NULL);
	thpool_add_work(thpool, (void*)sleep_2_secs, NULL);
	sleep(1);
	thpool_pause(thpool);
	count = thpool_num_jobs_placed(thpool);
	if(num !=2 ){
		printf("Expected 2 jobs placed, got %ld", count);
	}
	count = thpool_num_jobs_done(thpool);
	if(num != 0){
		printf("Expected 0 jobs done, got %ld", count);
	}
	thpool_resume(thpool);

	sleep(1);
	count = thpool_num_jobs_done(thpool);
	if(num !=2 ){
		printf("Expected 2 jobs done, got %ld", count);
	}

	// thpool_destroy(thpool);

	// sleep(1); // Sometimes main exits before thpool_destroy finished 100%

	return 0;
}
