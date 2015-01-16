#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

/* This showcasts this issue: https://sourceware.org/ml/glibc-bugs/2007-04/msg00036.html */
/* Also here: http://stackoverflow.com/questions/27803819/pthreads-leak-memory-even-if-used-correctly/27804629 */

volatile int threads_keepalive = 1;

void* thread_do(void *arg){
	while(threads_keepalive)
		sleep(1);
	pthread_exit(NULL);
}

int main(void){

	/* Make threads */
	pthread_t* threads;
	threads = malloc(2 * sizeof(pthread_t));
	pthread_create(&threads[0], NULL, &thread_do, NULL);
	pthread_create(&threads[1], NULL, &thread_do, NULL);
	pthread_detach(threads[0]);
	pthread_detach(threads[1]);
	sleep(1); // MAKING SURE THREADS HAVE INITIALIZED

	/* Kill threads */
	threads_keepalive = 0;
	sleep(3); // MAKING SURE THREADS HAVE UNBLOCKED
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	free(threads);

	return 0;
}
