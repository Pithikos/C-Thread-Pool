#! /bin/bash

#
# This will run all tests for a simple compilation
#



# ---------------------------- Tests -----------------------------------

DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

. $DIR/threadpool.sh
. $DIR/api.sh
. $DIR/pause_resume.sh
. $DIR/heap_stack_garbage.sh
. $DIR/memleaks.sh
. $DIR/wait.sh

echo "No errors"
