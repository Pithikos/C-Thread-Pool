#include <stdio.h>
#include "thpool.h"

pthread_mutex_t mutex_sum = PTHREAD_MUTEX_INITIALIZER;
int sum=0;


/*void add_num(int num) {
   // serialize access to sum variable (solves race conditions)
   pthread_mutex_lock(&mutex_sum);
   printf("%u: sum is %d\n", pthread_self(), sum);
   sum+=num;
   printf("%u: added %d\n", pthread_self(), num);
   pthread_mutex_unlock(&mutex_sum);
}*/



void task1() {
	puts("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&");
	pthread_mutex_lock(&mutex_sum);
	sum++;
	printf("%u: sum+1=%d\n", (int)pthread_self(), sum);
	pthread_mutex_unlock(&mutex_sum);
}


int main(){

	thpool_t* threadpool;
	threadpool=thpool_init(1);

   // Call a bunch of tasks
   thpool_add_work(threadpool, (void*)task1, NULL);
   printf("there are %d jobs in queue\n", jobqueue_len(threadpool));

   //puts("Waiting for work to finish");
   puts("MAIN THREAD SLEEPING 2 SECS");
   sleep(2);
   //thpool_wait(threadpool);
   //thpool_destroy(threadpool);
   
   
   printf("Sum is: %d\n", sum);
   return 0;
}
