/********************************** 
 * @author      Johan Hanssen Seferidis
 * License:     MIT
 * 
 **********************************/

#ifndef _THPOOL_
#define _THPOOL_





/* =================================== API ======================================= */


typedef struct thpool_* threadpool;

/**
 * @brief Thread pool worker
 *
 * The workers will get passed the threadpool, and a user supplied argument.
 * They can then repeatedly call thpool_get_work() on that pool, until
 * thpool_quitting() returns 0.
 */

typedef void (*thpool_worker)(threadpool,void*);

/**
 * @brief  Initialize threadpool
 * 
 * Initializes a threadpool. This function will not return untill all
 * threads have initialized successfully.
 * 
 * @example
 * 
 *    ..
 *    threadpool thpool;                     //First we declare a threadpool
 *    thpool = thpool_init(worker,4);               //then we initialize it to 4 threads
 *    ..
 * 
 * @param  num_threads   number of threads to be created in the threadpool
 * @param  worker    pointer to function to process work
 * @param  arg       argument to pass to worker on startup.
 * @return threadpool    created threadpool on success,
 *                       NULL on error
 */
threadpool thpool_init(int num_threads, thpool_worker worker, void* arg);

/**
 * @brief Add work to the job queue
 * 
 * Takes work and adds it to the threadpool's job queue.
 * If you want to add to work a function with more than one arguments then
 * a way to implement this is by passing a pointer to a structure.
 * 
 * NOTICE: You have to cast argument to not get warnings.
 * NOTICE: Any pointers added should be considered moved.
 * 
 * @example
 * 
 *    void print_num(int num){
 *       printf("%d\n", num);
 *    }
 * 
 *    int main() {
 *       ..
 *       thpool_init((void*)print_num, 1234);
 *       ..
 *       int a = 10;
 *       thpool_add_work(thpool, (void*)a);
 *       ..
 *    }
 * 
 * @param  threadpool    threadpool to which the work will be added
 * @param  arg_p         pointer to an argument
 * @return 0 on successs, -1 otherwise.
 */
int thpool_add_work(threadpool, void* arg_p);

/**
 * @brief Get work from the job queue
 * 
 * The worker for a thread should call this repeatedly, to get jobs
 * to perform. If it returns 0, that means the pool is shutting down
 * and the worker should cleanup, and exit.
 *
 * see thpool_add_work
 * 
 * @example
 * 
 *    void print_num(threadpool queue, int num){
 *       printf("%d\n", num);
 *       void* work;
 *       
 *       while(thpool_get_work(queue,&work)) {
 *         
 *         printf("work %d\n",(int)work);
 *       }
 *    }
 * 
 *    int main() {
 *       ..
 *       thpool_init((void*)print_num, 1234);
 *       ..
 *       int a = 10;
 *       thpool_add_work(thpool, (void*)a);
 *       ..
 *    }
 * 
 * @param  threadpool    threadpool to get work from.
 * @param  work          where to put the next job.
 * @return 0 if should exit, otherwise amount of retries waiting for jobs
 */
 
short thpool_get_work(threadpool queue, void** work);

/**
 * @brief Wait for all queued jobs to finish
 * 
 * Will wait for all jobs - both queued and currently running to finish.
 * Once the queue is empty and all work has completed, the calling thread
 * (probably the main program) will continue.
 * 
 * Smart polling is used in wait. The polling is initially 0 - meaning that
 * there is virtually no polling at all. If after 1 seconds the threads
 * haven't finished, the polling interval starts growing exponentially 
 * untill it reaches max_secs seconds. Then it jumps down to a maximum polling
 * interval assuming that heavy processing is being used in the threadpool.
 *
 * @example
 * 
 *    ..
 *    threadpool thpool = thpool_init(worker, 4);
 *    ..
 *    // Add a bunch of work
 *    ..
 *    thpool_wait(thpool);
 *    puts("All added work has finished");
 *    ..
 * 
 * @param threadpool     the threadpool to wait for
 * @return nothing
 */
void thpool_wait(threadpool);


/**
 * @brief Pauses all threads immediately
 * 
 * The threads will be paused no matter if they are idle or working.
 * The threads return to their previous states once thpool_resume
 * is called.
 * 
 * While the thread is being paused, new work can be added.
 * 
 * @example
 * 
 *    threadpool thpool = thpool_init(worker, 4);
 *    thpool_pause(thpool);
 *    ..
 *    // Add a bunch of work
 *    ..
 *    thpool_resume(thpool); // Let the threads start their magic
 * 
 * @param threadpool    the threadpool where the threads should be paused
 * @return nothing
 */
void thpool_pause(threadpool);


/**
 * @brief Unpauses all threads if they are paused
 * 
 * @example
 *    ..
 *    thpool_pause(thpool);
 *    sleep(10);              // Delay execution 10 seconds
 *    thpool_resume(thpool);
 *    ..
 * 
 * @param threadpool     the threadpool where the threads should be unpaused
 * @return nothing
 */
void thpool_resume(threadpool);


/**
 * @brief Destroy the threadpool
 * 
 * This will wait for the currently active threads to finish and then 'kill'
 * the whole threadpool to free up memory.
 * 
 * @example
 * int main() {
 *    threadpool thpool1 = thpool_init(worker, 2);
 *    threadpool thpool2 = thpool_init(worker, 2);
 *    ..
 *    thpool_destroy(thpool1);
 *    ..
 *    return 0;
 * }
 * 
 * @param threadpool     the threadpool to destroy
 * @return nothing
 */
void thpool_destroy(threadpool);




#endif
