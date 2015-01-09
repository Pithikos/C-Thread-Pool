/********************************** 
 * @author      Johan Hanssen Seferidis
 * @date        12/08/2011
 * License:     MIT
 * 
 **********************************/

/* Description: Library providing a threading pool where you can add work on the fly. The number
 *              of threads in the pool is adjustable when creating the pool. In most cases
 *              this should equal the number of threads supported by your cpu.
 *          
 *              For an example on how to use the threadpool, check the main.c file or just read
 *              the documentation found in the README.md file.
 * 
 *              In this header file a detailed overview of the functions and the threadpool's logical
 *              scheme is presented in case you wish to tweak or alter something. 
 * 
 * 
 * 
 *               _______________________________________________________        
 *             /                                                       \
 *             |   JOB QUEUE        | job1 | job2 | job3 | job4 | ..   |
 *             |                                                       |
 *             |   threadpool      | thread1 | thread2 | ..            |
 *             \_______________________________________________________/
 * 
 * 
 *    Description:       Jobs are added to the job queue. Once a thread in the pool
 *                       is idle, it is assigned with the first job from the queue(and
 *                       erased from the queue). It's each thread's job to read from 
 *                       the queue serially(using lock) and executing each job
 *                       until the queue is empty.
 * 
 * 
 *    Scheme:
 * 
 *    thpool______                jobqueue____                      ______ 
 *    |           |               |           |       .----------->|_job0_| Newly added job
 *    |           |               |  rear  ----------'             |_job1_|
 *    | jobqueue----------------->|           |                    |_job2_|
 *    |           |               |  front ----------.             |__..__| 
 *    |___________|               |___________|       '----------->|_jobn_| Job for thread to take
 * 
 * 
 *    job0________ 
 *    |           |
 *    | function---->
 *    |           |
 *    |   arg------->
 *    |           |         job1________ 
 *    |  next-------------->|           |
 *    |___________|         |           |..
 * 
 * 
 * 
 */


#ifndef _THPOOL_

#define _THPOOL_

#include <pthread.h>
#include <semaphore.h>





/* ========================== STRUCTURES ============================ */


/* Binary semaphore */
typedef struct bsem_t {
	pthread_mutex_t mutex;
	pthread_cond_t   cond;
	int v;
} bsem_t;


/* Job */
typedef struct job_t{
	void*  (*function)(void* arg);       /* function pointer          */
	void*          arg;                  /* function's argument       */
	struct job_t*  prev;                 /* pointer to previous job   */
} job_t;


/* Job queue */
typedef struct jobqueue_t{
	pthread_mutex_t  rwmutex;            /* used for queue r/w access */
	job_t           *front;              /* pointer to front of queue */
	job_t           *rear;               /* pointer to rear  of queue */
	bsem_t          *has_jobs;           /* flag as binary semaphore  */
	int              len;                /* number of jobs in queue   */
} jobqueue_t;


/* Thread */
typedef struct thread_t{
	int       id;                       /* friendly id                */
	int       initialized;              /* binary to solve race conds */
	pthread_t pthread;                  /* pointer to actual thread   */
	struct thpool_t* thpool;            /* access to thpool           */
} thread_t;


/* Threadpool */
typedef struct thpool_t{
	thread_t**       threads;            /* pointer to threads        */
	int              threads_alive;      /* threads currently alive   */
	pthread_mutex_t  thcount_lock;       /* used for thread count etc */
	jobqueue_t*      jobqueue;           /* pointer to the job queue  */    
} thpool_t;





/* =========================== FUNCTIONS ============================ */


/* ---------------------- Threadpool specific ----------------------- */

/**
 * @brief  Initialize threadpool
 * 
 * Initializes a threadpool. This function will not return untill all
 * threads have initialized succesfully.
 * 
 * @param  number of threads to be used
 * @return threadpool struct on success,
 *         NULL on error
 */
thpool_t* thpool_init(int threadsN);


/**
 * @brief Add work to the job queue
 * 
 * Takes an action and its argument and adds it to the threadpool's job queue.
 * If you want to add to work a function with more than one arguments then
 * a way to implement this is by passing a pointer to a structure.
 * 
 * NOTICE: You have to cast both the function and argument to not get warnings.
 * 
 * @param  threadpool to where the work will be added to
 * @param  function to add as work
 * @param  argument to the above function
 * @return int
 */
int thpool_add_work(thpool_t* thpool, void *(*function_p)(void*), void* arg_p);


/**
 * @brief Wait for all jobs in job queue to finish
 * 
 * Will wait for all jobs in the queue to finish. Polling is used for this.
 * 
 * @param  threadpool to wait for
 * @return nothing
 */
void thpool_wait(thpool_t* thpool);


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
void thpool_pause(thpool_t* thpool);


/**
 * @brief Unpauses all threads if they are paused
 * 
 * @param  threadpool where the threads should be unpaused
 * @return nothing
 */
void thpool_resume(thpool_t* thpool);


/**
 * @brief Destroy the threadpool
 * 
 * This will 'kill' the threadpool and free up memory. If threads are active when this
 * is called, they will finish what they are doing and then they will get destroyied.
 * 
 * @param threadpool a pointer to the threadpool structure you want to destroy
 * @return nothing
 */
void thpool_destroy(thpool_t* thpool);



/* ----------------------- Thread specific --------------------------- */


/**
 * @brief Initialize a thread in the thread pool
 * 
 * Will initialize a new thread for the given threadpool and give the
 * the thread an ID
 * 
 * Notice also that the thread's id is not populated automatically.
 * 
 * @param threadpool    threadpool to create thread
 * @param thread        address to the pointer of the thread to be created
 * @param id            id to be given to thread
 * 
 */
void thread_init(thpool_t* thpool, thread_t** thread, int id);


/**
 * @brief What each thread is doing
 * 
 * In principle this is an endless loop. The only time this loop gets interuppted is once
 * thpool_destroy() is invoked.
 * 
 * @param threadpool to use
 * @return nothing
 */
static void* thread_do(thread_t* thread);


/**
 * @brief Sets the calling thread on hold until threads_on_hold is set to 1
 * @param nothing
 * @return nothing
 */
static void thread_hold();


/**
 * @brief Frees a thread
 * @param thread
 * @return nothing
 */
static void thread_destroy(thread_t* thread);





/* ----------------------- Queue specific --------------------------- */


/**
 * @brief  Initialize queue
 * @param  pointer to threadpool
 * @return 0 on success,
 *        -1 on memory allocation error
 */
static int jobqueue_init(thpool_t* thpool);


/**
 * @brief  Clears the queue
 * @param  pointer to threadpool
 * @return nothing
 */
static void jobqueue_clear(thpool_t* thpool);


/**
 * @brief Add job to queue
 * 
 * A new job will be added to the queue. The new job MUST be allocated
 * before passed to this function or else other functions like jobqueue_empty()
 * will be broken.
 * 
 * MUST HOLD MUTEX WHEN CALLING
 * 
 * @param pointer to threadpool
 * @param pointer to the new job(MUST BE ALLOCATED)
 * @return nothing 
 */
static void jobqueue_push(thpool_t* thpool, job_t* newjob);


/**
 * @brief Get first job from queue(removes it from queue)
 * 
 * This does not free allocated memory so be sure to have peeked() \n
 * before invoking this as else there will result lost memory pointers.
 * 
 * MUST HOLD MUTEX WHEN CALLING
 * 
 * @param  pointer to threadpool
 * @return point to job on success,
 *         NULL if there is no job in queue
 */
static job_t* jobqueue_pull(thpool_t* thpool);


/**
 * @brief Remove and deallocate all jobs in queue
 * 
 * This function will deallocate all jobs in the queue and set the
 * jobqueue to its initialization values, thus tail and head pointing
 * to NULL and amount of jobs equal to 0.
 * 
 * @param pointer to threadpool structure
 * */
static void jobqueue_destroy(thpool_t* thpool);





/* ----------------------- Synchronisation -------------------------- */


/**
 * @brief Inits semaphore to given value
 * @param binary semaphore
 * @param value 1 or 0
 * */
static void bsem_init(bsem_t *bsem, int value);


/**
 * @brief Resets semaphore to 0
 * @param binary semaphore
 * */
static void bsem_reset(bsem_t *bsem);


/**
 * @brief Sets semaphore to one and notifies at least one thread
 * @param binary semaphore
 * */
static void bsem_post(bsem_t *bsem);


/**
 * @brief Sets semaphore to one and notifies all threads
 * @param binary semaphore
 * */
static void bsem_post_all(bsem_t *bsem);


/**
 * @brief Waits on semaphore until semaphore has value 0
 * @param binary semaphore
 * */
static void bsem_wait(bsem_t *bsem);


#endif
