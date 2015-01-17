# C Thread Pool

This is a minimal but fully functional threadpool implementation.

  * ANCI C and POSIX compliant
  * Number of threads can be chosen on initialization
  * Minimal but powerful interface
  * Full documentation

The threadpool is under MIT license. Notice that this project took a considerable amount of work and sacrifice of my free time and the reason I give it for free (even for commercial use) is so when you become rich and wealthy you don't forget about us open-source creatures of the night. Cheers!


## v2 updates

This is an updated and heavily refactored version of my original threadpool. The main points taken into consideration into this new version are:

  * Synchronisation control from the user (pause/resume/wait)
  * Thorough testing for memory leaks and race conditions
  * Cleaner and more opaque API


## Compiling

The library is not precompiled so you have to compile it with your project. The thread pool
uses POSIX threads so if you compile with **gcc** on Linux you have to use the flag `-pthread` like this:

    gcc example.c thpool.c -pthread -o example


Then run the executable like this:

    ./example


##Basic usage

1. Include the header in your source file: ``#include "thpool.h"`
2. Make a thread pool with 4 threads: `threadpool thpool = thpool_init(4);`
3. Add work to the pool: `thpool_add_work(thpool, (void*)doSth, (void*)arg);`

The workers will start their work automatically as fast as there is new work
added. If you want to wait for all added work to be finished before continuing
you can use `thpool_wait(thpool);`. If you want to destroy the pool you can use
`thpool_destroy(thpool);`.



##API

For a deeper look into the documentation check in the `thpool.h` file. Also notice that to use **any** of the API you have to **include the thpool.h**.

| Function example                                  | Description                   |
|---------------------------------------------------|-------------------------------|
| thpool_init(4)                                    | Will return a new threadpool with 4 threads.         |
| thpool_add_work(thpool, void *(*function_p)(void*), void* arg_p) | Adds work to the threadpool. Work is simply a function. You can pass a single argument to the function if you wish. If not, NULL should be passed. |
| thpool_wait(thpool)                               | Will wait for all jobs (both in queue and currently running) to finish.    |
| thpool_destroy(thpool)                            | This will destroy thpool. If jobs are currently being executed, then it will wait for them to finish before destroying the threadpool. |
| thpool_pause(thpool)                              | All threads in the threadpool will pause no matter if they are idle or executing |
| thpool_pause(thpool)                              | If the threadpool is paused, then all threads will resume from where they were |
