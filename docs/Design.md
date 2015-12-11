## High level
	
	Description: Library providing a threading pool where you can add work on the fly. The number
	             of threads in the pool is adjustable when creating the pool. In most cases
	             this should equal the number of threads supported by your cpu.
	         
	             For an example on how to use the threadpool, check the main.c file or just read
	             the documentation found in the README.md file.
	
	             In this header file a detailed overview of the functions and the threadpool's logical
	             scheme is presented in case you wish to tweak or alter something. 
	
	
	
	              _______________________________________________________        
	            /                                                       \
	            |   JOB QUEUE        | job1 | job2 | job3 | job4 | ..   |
	            |                                                       |
	            |   threadpool      | thread1 | thread2 | ..            |
	            \_______________________________________________________/
	
	
	   Description:       Jobs are added to the job queue. Once a thread in the pool
	                      is idle, it is assigned with the first job from the queue(and
	                      erased from the queue). It's each thread's job to read from 
	                      the queue serially(using lock) and executing each job
	                      until the queue is empty.
	
	
	   Scheme:
	
	   thpool______                jobqueue____                      ______ 
	   |           |               |           |       .----------->|_job0_| Newly added job
	   |           |               |  rear  ----------'             |_job1_|
	   | jobqueue----------------->|           |                    |_job2_|
	   |           |               |  front ----------.             |__..__| 
	   |___________|               |___________|       '----------->|_jobn_| Job for thread to take
	
	
	   job0________ 
	   |           |
	   | function---->
	   |           |
	   |   arg------->
	   |           |         job1________ 
	   |  next-------------->|           |
	   |___________|         |           |..
