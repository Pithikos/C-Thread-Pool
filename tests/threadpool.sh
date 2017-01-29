#! /bin/bash

#
# This file has several functional tests similar to what a user
# might use in his/her code
#

DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

. $DIR/funcs.sh


function test_mass_addition { #endsum #threads
	echo "Adding up to $1 with $2 threads"
	compile $DIR/src/conc_increment.c
	output=$($DIR/test $1 $2)
	num=$(echo $output | awk '{print $(NF)}')
	if [ "$num" == "$1" ]; then
		return
	fi
	err "Expected $1 but got $output" "$output"
	exit 1
}


# Run tests
test_mass_addition 100 4
test_mass_addition 100 100
test_mass_addition 100000 100

echo "No errors"
