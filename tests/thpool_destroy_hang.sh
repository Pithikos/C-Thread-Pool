#! /bin/bash
#
# Test for the deterministic thpool_destroy() hang
#

. funcs.sh

# -------------------------------------------------- #
# Single test: must finish without hitting SIGALRM   #
# -------------------------------------------------- #
function test_thpool_destroy_hang {
    compile src/thpool_destroy_hang.c
    echo "Testing for thpool_destroy() hang"
    output=$(timeout 2 ./test)
    if [[ $? != 0 ]]; then
        err "thpool_destroy() hang reproduced" "$output"
        exit 1
    fi

}

# Run the test
test_thpool_destroy_hang

echo "No errors"
