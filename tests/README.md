Tests
------------------------------------------------------------------------

**Case tests**
````
memleaks     - Will run tests for memory leaks. valgrind is being used for this.
               Notice that valgrind requires one second to init each thread.
threadpool   - Will run general functional tests for the threadpool.
pause_resume - Will test the synchronisation of the threadpool from the user.
wait         - Will run tests to assuse that the wait() function works correctly.

````


**Compilation cases**
````
normal_compile    - Will run all tests above against a simply compiled threadpool.
optimized_compile - Will run all tests but against a binary that was compiled
                    with optimization flags.      
````


**On errors**

Check the created log file `error.log`
