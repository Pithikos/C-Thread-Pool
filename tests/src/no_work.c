#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "../../thpool.h"


int main(int argc, char *argv[]){
	
	char* p;
	if (argc != 2){
		puts("This testfile needs excactly one arguments");
		exit(1);
	}
	int threads = strtol(argv[1], &p, 10);

	thpool_t* thpool;
	thpool = thpool_init(threads);
	thpool_destroy(thpool);

	return 0;
}
