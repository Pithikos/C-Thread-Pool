#! /bin/bash

#
# This will run all tests for a simple compilation
#



# ---------------------------- Tests -----------------------------------

. threadpool.sh
. pause_resume.sh
. heap_stack_garbage.sh
. memleaks.sh
. wait.sh

echo "No errors"
