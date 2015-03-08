#! /bin/bash

#
# This file tests for possible bugs
#

. funcs.sh


# ---------------------------- Tests -----------------------------------

function test_with_nonzero_heap_and_stack {
    compile src/nonzero_heap_stack.c
    if ! timeout 1 ./test; then
        err "Fail running on nonzero heap and stack"
        exit 1
    else
        return
    fi
}


# Run tests
test_with_nonzero_heap_and_stack

echo "No errors"
