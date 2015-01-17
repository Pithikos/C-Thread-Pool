# C Thread Pool

This is a minimal but fully functional threadpool implementation.

  * ANCI C and POSIX compliant
  * Number of threads can be chosen on initialization
  * Minimal interface
  * Full documentation

The threadpool is under MIT license. Notice that this project took a considerable amount of work and sacrifice of my free time and the reason I give it for free (even for commercial use) is so when you become rich and wealthy you don't forget about us open-source creatures. Cheers!


## v2 updates

This is an updated and heavily refactored version of my original threadpool. The main points taken into consideration into this new version are:

  * Synchronisation control from the user
  * Thorough testing for memory leaks and race conditions
  * Cleaner and more opaque API


## Compiling

The library is not precompiled so you have to compile it with your project. The thread pool
uses POSIX threads so if you compile with **gcc** on Linux you have to use the flag `-pthread` like this:

    gcc example.c thpool.c -pthread -o test


Then run the executable like this:

    ./test


##Basic usage

1. Include the header in your source file: ``#include "thpool.h"`
2. Make a thread pool with 4 threads: `threadpool thpool = thpool_init(4);`
3. Add work to the pool: `thpool_add_work(thpool, (void*)doSth, (void*)arg);`

The workers will start their work automatically as fast as there is new work
added. If you want to wait for all added work to be finished before continuing
you can use `thpool_wait(thpool);`. If you want to destroy the pool you can use
`thpool_destroy(thpool);`.



##Threadpool Interface

For a deeper look into the documentation check in the `thpool.h` file. Also notice that to use **any** of the API you have to **include the thpool.h**.

| Function example                                                 | Description                                                                                                                                        |
|------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------|
| thpool_init(4)                                                   | Will return a new threadpool with 4 threads.                                                                                                       |
| thpool_add_work(thpool, void *(*function_p)(void*), void* arg_p) | Adds work to the threadpool. Work is simply a function. You can pass a single argument to the function if you wish. If not, NULL should be passed. |
| thpool_wait(thpool)                                              | Will wait for all jobs (both in queue and currently running) to finish.                                                                            |
| thpool_destroy(thpool)                                           | This will destroy thpool. If jobs are currently being executed, then it will wait for them to finish before destroying the threadpool.             |



````
NAME
     thpool_t* thpool_init(int num_of_threads);

SYNOPSIS
  
     #include <thpool.h>

     threadpool thpool_init(int num_threads);

DESCRIPTION

     Initialises the threadpool. On success a threadpool structure is returned.
     Otherwise if memory could not be allocated NULL is returned. The argument
     which is the number of threads in the threadpool should be a thoughfull
     choice. A common suggestion is to use as many threads as the ones supported
     by your cpu.

     Exampl
     threadpool thpool;                      //First we declare a threadpool
     thpool=thpool_init(4);                  //then we initialise it to 4 threads
````

-----------------------------------------------------------------------------------


```
NAME
     thpool_add_work(threadpool thpool, void *(*function_p)(void*), void* arg_p);

SYNOPSIS
  
     #include <thpool.h>

     int thpool_add_work(threadpool thpool, void *(*function_p)(void*), void* arg_p);

DESCRIPTION

     Adds work to the thread pool. Work is concidered an individual function with an
     argument. First argument is a pointer to the pool itself. The second argument is
     a pointer to a function and third argument is a pointer to an argument. To pass
     multiple arguments just use a struct. If the function you want to pass doesn't
     fit the parameters of this prototype, use casting. If your function or argument
     doesn't fit the parameters' and return's value type then you should use casting
     to avoid warnings from the compiler.

     Example:
     void printSth(char* str);                            //Prints a text on the screen
     thpool_add_work(thpool, (void*)printSth, (void*)str);//Pay attention to the casting
````


-----------------------------------------------------------------------------------


````
NAME
     void thpool_wait(threadpool thpool);

SYNOPSIS
  
     #include <thpool.h>

     void thpool_wait(threadpool thpool);

DESCRIPTION

     This function will block the main thread until all jobs in the the thread pool
     has been finished. Polling is being used for this. By default the POLLING_INTERVAL
     is set to one second.

     Example:
     thpool_wait(thpool);                //thpool is of type threadpool
````

-----------------------------------------------------------------------------------


````
NAME
     void thpool_destroy(threadpool thpool);

SYNOPSIS
  
     #include <thpool.h>

     void thpool_destroy(threadpool thpool);

DESCRIPTION

     This function will destroy the given threadpool. If some threads are working in the pool
     then thpool_destroy() will wait for them to finish. Once they are finished the
     threadpool is deallocated releasing all resources back to the system.

     Example:
     thpool_destroy(thpool);           //thpool is of type threadpool
````
