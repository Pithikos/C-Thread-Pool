#! /bin/bash

#
# This file has several tests to check for memory leaks.
# valgrind is used so make sure you have it installed
#

. funcs.sh


# ---------------------------- Tests -----------------------------------


function test_pause_resume_est7secs { #threads
	echo "Pause and resume test for 7 secs with $1 threads"
	compile src/pause_resume.c
	realsecs=$(/usr/bin/time -f '%e' ./test "$1" 2>&1 > /dev/null)
	threshold=1.00 # in secs
	
	ret=$(python -c "print(($realsecs-7)<=$threshold)")

	if [ "$ret" == "True" ]; then
		return
	fi
	err "Elapsed $realsecs which is more than than allowed"
	exit 1
}



# Run tests
test_pause_resume_est7secs 4



echo "No pause/resume errors"
