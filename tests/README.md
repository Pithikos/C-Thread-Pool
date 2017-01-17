Tests
------------------------------------------------------------------------

**Test cases**
````
memleaks           - Will run tests for memory leaks. valgrind is being used for this.
                     Notice that valgrind requires one second to init each thread.
threadpool         - Will run general functional tests for the threadpool.
pause_resume       - Will test the synchronisation of the threadpool from the user.
wait               - Will run tests to ensure that the wait() function works correctly.
heap_stack_garbage - Will test if previous garbage affects new threapools created.
````
Any test can be run with extra flags by exporting the variable COMPILATION_FLAGS. That's
also how the optimized_compile test works.


**Compilation cases**
````
normal_compile     - Will run all tests above against a simply compiled threadpool.
optimized_compile  - Will run all tests but against a binary that was compiled
                     with optimization flags.      
````


**On errors**

Check the created log file `error.log`
