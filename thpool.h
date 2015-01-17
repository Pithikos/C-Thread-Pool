/********************************** 
 * @author      Johan Hanssen Seferidis
 * @date        12/08/2011
 * License:     MIT
 * 
 **********************************/

#ifndef _THPOOL_

#define _THPOOL_

#include <pthread.h>
#include <semaphore.h>


/* thpool is a pointer to a thpool data structure */
typedef struct thpool_* threadpool;


/* =========================== FUNCTIONS ============================ */


/**
 * @brief  Initialize threadpool
 * 
 * Initializes a threadpool. This function will not return untill all
 * threads have initialized successfully.
 * 
 * @param  num_threads   number of threads to be created in the threadpool
 * @return threadpool    created threadpool on success,
 *                       NULL on error
 */
threadpool thpool_init(int num_threads);


/**
 * @brief Add work to the job queue
 * 
 * Takes an action and its argument and adds it to the threadpool's job queue.
 * If you want to add to work a function with more than one arguments then
 * a way to implement this is by passing a pointer to a structure.
 * 
 * NOTICE: You have to cast both the function and argument to not get warnings.
 * 
 * @param  threadpool    threadpool to which the work will be added
 * @param  function_p    pointer to function to add as work
 * @param  arg_p         pointer to an argument
 * @return nothing
 */
int thpool_add_work(threadpool, void *(*function_p)(void*), void* arg_p);


/**
 * @brief Wait for all queued jobs to finish
 * 
 * Will wait for all jobs - both queued and currently running to finish.
 * Once the queue is empty and all work has completed, the calling thread
 * (probably the main program) will continue.
 * 
 * Polling is used in wait. By default the polling interval is one second.
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
 * @param threadpool    the threadpool where the threads should be paused
 * @return nothing
 */
void thpool_pause(threadpool);


/**
 * @brief Unpauses all threads if they are paused
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
 * @param threadpool     the threadpool to destroy
 * @return nothing
 */
void thpool_destroy(threadpool);




#endif
