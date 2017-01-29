#! /bin/bash

#
# This file has several tests to check that the API
# works to an acceptable standard.
#

DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

. $DIR/funcs.sh


# ---------------------------- Tests -----------------------------------


function test_api {
	echo "Testing API calls.."
	compile $DIR/src/api.c
	output=`$DIR/test`
	if [[ $? != 0 ]]; then
		 err "$output" "$output"
		 exit 1
	fi
}



# Run tests
test_api





echo "No API errors"
