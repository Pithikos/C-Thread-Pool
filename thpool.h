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
typedef struct thpool_* thpool;


/* =========================== FUNCTIONS ============================ */


/**
 * @brief  Initialize threadpool
 * 
 * Initializes a threadpool. This function will not return untill all
 * threads have initialized successfully.
 * 
 * @param  num_threads   number of threads to be created in the threadpool
 * @return thpool        pointer to created threadpool on success,
 *                       pointer to NULL on error
 */
extern thpool thpool_init(int num_threads);


/**
 * @brief Add work to the job queue
 * 
 * Takes an action and its argument and adds it to the threadpool's job queue.
 * If you want to add to work a function with more than one arguments then
 * a way to implement this is by passing a pointer to a structure.
 * 
 * NOTICE: You have to cast both the function and argument to not get warnings.
 * 
 * @param  thpool        threadpool to which the work will be added
 * @param  function      function to add as work
 * @param  argument      single argument to passed function
 * @return nothing
 */
extern int thpool_add_work(thpool, void *(*function)(void*), void* arg_p);


/**
 * @brief Wait for all queued jobs to finish
 * 
 * Will wait for all jobs - both queued and currently running to finish.
 * Once the queue is empty and all work has completed, the calling thread
 * (probably the main program) will continue.
 * 
 * Polling is used in wait. By default the polling interval is one second.
 *
 * @param  threadpool to wait for
 * @return nothing
 */
extern void thpool_wait(thpool);


/**
 * @brief Pauses all threads immediately
 * 
 * The threads will be paused no matter if they are idle or working.
 * The threads return to their previous states once thpool_resume
 * is called.
 * 
 * While the thread is being paused, new work can be added.
 * 
 * @param  threadpool where the threads should be paused
 * @return nothing
 */
extern void thpool_pause(thpool);


/**
 * @brief Unpauses all threads if they are paused
 * 
 * @param thpool     the threadpool where the threads should be unpaused
 * @return nothing
 */
extern void thpool_resume(thpool);


/**
 * @brief Destroy the threadpool
 * 
 * This will wait for the currently active threads to finish and then 'kill'
 * the whole threadpool to free up memory.
 * 
 * @param thpool     the threadpool to destroy
 * @return nothing
 */
extern void thpool_destroy(thpool);




#endif
