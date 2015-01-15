/* 
 * This is just an example on how to use the thpool library 
 * 
 * We create a pool of 4 threads and then add 20 tasks to the pool(10 task1 
 * functions and 10 task2 functions).
 * 
 * Task1 doesn't take any arguments. Task2 takes an integer. Task2 is used to show
 * how to add work to the thread pool with an argument.
 * 
 * As soon as we add the tasks to the pool, the threads will run them. One thread
 * may run x tasks in a row so if you see as output the same thread running several
 * tasks, it's not an error.
 * 
 * All jobs will not be completed and in fact maybe even none will. You can add a sleep()
 * function if you want to complete all tasks in this test file to be able and see clearer
 * what is going on.
 * 
 * */

#include <stdio.h>

#include "thpool.h"



/* Some arbitrary task 1 */
void task1(){
	printf("# Thread working: %u\n", (int)pthread_self());
	printf("  Task 1 running..\n");
}



/* Some arbitrary task 2 */
void task2(int a){
	printf("# Thread working: %u\n", (int)pthread_self());
	printf("  Task 2 running..\n");
	printf("%d\n", a);
}



int main(){
	int i;
	
	thpool_t* threadpool;             /* make a new thread pool structure     */
	threadpool=thpool_init(4);        /* initialise it to 4 number of threads */


	puts("Adding 20 tasks to threadpool");
	int a=54;
	for (i=0; i<10; i++){
		thpool_add_work(threadpool, (void*)task1, NULL);
		thpool_add_work(threadpool, (void*)task2, (void*)a);
	};


    puts("Will kill threadpool");
	thpool_destroy(threadpool);
	
	return 0;
}
