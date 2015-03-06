#! /bin/bash

#
# This file has several functional tests similar to what a user
# might use in his/her code
#

. funcs.sh


function test_mass_addition { #endsum #threads
	echo "Adding up to $1 with $2 threads"
	compile src/conc_increment.c
	output=$(./test $1 $2)
	num=$(echo $output | awk '{print $(NF)}')
	if [ "$num" == "$1" ]; then
		return
	fi
	err "Expected $1 but got $output" "$output"
	exit 1
}


# Run tests
test_mass_addition 100 4
test_mass_addition 100 1000
test_mass_addition 100000 1000

echo "No errors"
