### Why isn't `pthread_exit()` used to exit a thread?

`thread_do` used to use `pthread_exit()`. However that resulted in
hard times of testing for memory leaks. The reason is that on `pthread_exit()`
not all memory is freed bt pthread (probably for future threads or false
belief that the application is terminating). For these reasons a simple return
is used.

Interestingly using `pthread_exit()` results in much more memory being allocated.


### Why do you use `sleep()` after calling `thpool_destroy()`?

This is needed only in the tests. The reason is that if you call `thpool_destroy()`
and then exit immediately, maybe the program will exit before all the threads
had the time to deallocate. In that way it is impossible to check for memory
leaks.

In production you don't have to worry about this since if you call `exit()`,
immediately after you destroyed the pool, the threads will be freed
anyway by the OS. If you anyway destroy the pool in the middle of your
program it doesn't matter again since the program will not exit immediately
and thus threads will have more than enough time to terminate.


### Why does `wait()` use all my CPU?

Notice: As of 11-Dec-2015 `wait()` doesn't use polling anymore. Instead a conditional variable is being used so in theory there should not be any CPU overhead.

Normally `wait()` will spike CPU usage to full when called. This is normal as long as it doesn't last for more than 1 second. The reason this happens is that `wait()` goes through various phases of polling (what is called smart polling).

 * Initially there is no interval between polling and hence the 100% use of your CPU.
 * After that the polling interval grows exponentially.
 * Finally after x seconds, if there is still work, polling falls back to a very big interval.

The reason `wait()` works in this way, is that the function is mostly used when someone wants to wait for some calculation to finish. So if the calculation is assumed to take a long time then we don't want to poll too often. Still we want to poll fast in case the calculation is a simple one. To solve these two problems, this seemingly awkward behaviour is present.
