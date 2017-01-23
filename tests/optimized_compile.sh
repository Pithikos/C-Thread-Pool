#! /bin/bash

#
# This file will run all tests but with a binary that has
# been compiled with optimization flags.
#



# ---------------------------- Tests -----------------------------------

DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi

COMPILATION_FLAGS='-g -O'
. $DIR/normal_compile.sh

echo "No optimization errors"
