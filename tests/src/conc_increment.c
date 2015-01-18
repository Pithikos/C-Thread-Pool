#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "../../thpool.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int sum=0;


void increment() {
	pthread_mutex_lock(&mutex);
	sum ++;
	pthread_mutex_unlock(&mutex);
}


int main(int argc, char *argv[]){
	
	char* p;
	if (argc != 3){
		puts("This testfile needs excactly two arguments");
		exit(1);
	}
	int num_jobs    = strtol(argv[1], &p, 10);
	int num_threads = strtol(argv[2], &p, 10);

	threadpool thpool = thpool_init(num_threads);
	
	int n;
	for (n=0; n<num_jobs; n++){
		thpool_add_work(thpool, (void*)increment, NULL);
	}
	
	thpool_wait(thpool);

	printf("%d\n", sum);

	return 0;
}
