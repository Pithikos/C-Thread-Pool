##Why not use `pthread_exit()`
`thread_do` used to use pthread_exit(). However that resulted in
hard times of testing for memory leaks. The reason is that on pthread_exit()
not all memory is freed bt pthread (probably for future threads or false
belief that the application is terminating). For these reasons a simple return
is used.

Interestingly using `pthread_exit()` results in much more memory being allocated.


##Why `sleep()` after `thpool_destroy()`?
This is needed only in the tests. The reason is that if you call thpool_destroy
and then exit immedietely, maybe the program will exit before all the threads
had the time to deallocate. In that way it is impossible to check for memory
leaks.

In production you don't have to worry about this since if you call exit,
immedietely after you destroyied the pool, the threads will be freed
anyway by the OS. If you eitherway destroy the pool in the middle of your
program it doesn't matter again since the program will not exit immediately
and thus threads will have more than enough time to terminate.
