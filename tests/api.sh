#! /bin/bash

#
# This file has several tests to check that the API
# works to an acceptable standard.
#

. funcs.sh


# ---------------------------- Tests -----------------------------------


function test_api {
	echo "Testing API calls.."
	compile src/api.c
	output=`./test`
	if [[ $? != 0 ]]; then
		 err "$output" "$output"
		 exit 1
	fi
}



# Run tests
test_api





echo "No API errors"
