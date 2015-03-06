#! /bin/bash

#
# This file has several tests to check for memory leaks.
# valgrind is used so make sure you have it installed
#

. funcs.sh


# ---------------------------- Tests -----------------------------------


function test_wait_each_job { #threads #jobs
	echo "Will test waiting for each job ($1 threads, $2 jobs)"
	compile src/wait.c
	realsecs=$(time_exec ./test $2 $1 1)
	threshold=1.00 # in secs

	ret=$(python -c "print((abs($realsecs-$2))<=$threshold)")

	if [ "$ret" == "True" ]; then
		return
	fi
	err "Elapsed $realsecs which is more than than allowed"
	exit 1
}


function test_wait_pool { #threads #jobs
	echo "Will test waiting for whole threadpool ($1 threads, $2 jobs)"
	compile src/wait.c
	realsecs=$(time_exec ./test $2 $1 0)
	threshold=1.00 # in secs
	
	expected_time=$(python -c "import math; print(math.ceil($2/$1.0))")
	ret=$(python -c "print((abs($realsecs-$expected_time))<=$threshold)")
	
	if [ "$ret" == "True" ]; then
		return
	fi
	err "Elapsed $realsecs which is too different from what expected ($expected_time)"
	exit 1
}


# Run tests
test_wait_each_job 1 4
test_wait_each_job 4 4
test_wait_each_job 4 10
test_wait_pool 1 4
test_wait_pool 8 2
test_wait_pool 4 4
test_wait_pool 4 20

echo "No errors"
