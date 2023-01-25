#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "../../thpool.h"

/* 
 * THIS TEST NEEDS TO BE TIMED TO BE MEANINGFULL
 * 
 * main:    sleep 3 secs   sleep 2 secs
 *                      
 * thpool:                 sleep 4 secs
 * 
 * Thus the program should take just a bit more than 7 seconds.
 * 
 * */

void sleep_4_secs(){
	sleep(4);
	puts("SLEPT");
}

int main(int argc, char *argv[]){

	char* p;
	if (argc != 2){
		puts("This testfile needs exactly one arguments");
		exit(1);
	}
	int num_threads = strtol(argv[1], &p, 10);

	threadpool thpool = thpool_init(num_threads);
	
	thpool_pause(thpool);
	
	// Since pool is paused, threads should not start before main's sleep
	thpool_add_work(thpool, (void*)sleep_4_secs, NULL);
	thpool_add_work(thpool, (void*)sleep_4_secs, NULL);
	
	sleep(3);
	
	// Now we will start threads in no-parallel with main
	thpool_resume(thpool);

	sleep(2); // Give some time to threads to get the work
	
	thpool_destroy(thpool); // Wait for work to finish

	return 0;
}
