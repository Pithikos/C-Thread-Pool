#! /bin/bash

#
# This file tests for possible bugs
#

. funcs.sh


# ---------------------------- Tests -----------------------------------

function test_with_nonzero_heap_and_stack {
    compile src/nonzero_heap_stack.c
    echo "Testing for non-zero heap and stack"
    output=$(timeout 1 ./test)
    if [[ $? != 0 ]]; then
        err "Fail running on nonzero heap and stack" "$output"
        exit 1
    fi
}


# Run tests
test_with_nonzero_heap_and_stack

echo "No errors"
