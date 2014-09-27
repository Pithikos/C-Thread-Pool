````
Author:  Johan Hanssen Seferidis
Created: 2011-08-12
````


Compiling
========================================================================

The library is not precompiled so you have to compile it with your project. The thread pool
uses POSIX threads so if you compile with gcc you have to use the flag -pthread like this:

    gcc main.c thpool.c -pthread -o test


Then run the executable like this:

    ./test


Usage
========================================================================

1. Make a thread pool: `thpool_t* thpool;`
2. Initialise the thread pool with number of threads(workers) you want: `thpool=thpool_init(4);`
3. Add work to the pool: `thpool_add_work(thpool, (void*)doSth, (void*)arg);`
4. Destroy pool: `thpool_destroy(thpool);`


Threadpool Interface
========================================================================

````
NAME
     thpool_t* thpool_init(int num_of_threads);

SYNOPSIS
  
     #include <thpool.h>

     thpool_t* thpool_init(int num_of_threads);

DESCRIPTION

     Initialises the threadpool. On success a threadpool structure is returned.
     Otherwise if memory could not be allocated NULL is returned. The argument
     which is the number of threads in the threadpool should be a thoughfull
     choice. A common suggestion is to use as many threads as the ones supported
     by your cpu.

     Example:
     thpool_t* myThreadpool;                 //First we declare a threadpool
     myThreadpool=thpool_init(4);            //then we initialise it to 4 threads
````

-----------------------------------------------------------------------------------


```
NAME
     thpool_add_work(thpool_t* thpool, void *(*function_p)(void*), void* arg_p);

SYNOPSIS
  
     #include <thpool.h>

     int thpool_add_work(thpool_t* thpool, void *(*function_p)(void*), void* arg_p);

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
     void thpool_destroy(thpool_t* tp_p);

SYNOPSIS
  
     #include <thpool.h>

     void thpool_destroy(thpool_t* tp_p);

DESCRIPTION

     This function will destroy a threadpool. If some threads are working in the pool
     then thpool_destroy() will wait for them to finish. Once they are finished the
     threadpool is deallocated releasing all resources back to the system.

     Example:
     thpool_destroy(threadpool_p);           //threadpool_p being a pointer to a thpool_t
````
