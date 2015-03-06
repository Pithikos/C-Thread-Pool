#! /bin/bash

#
# This file will run all tests but with a binary that has
# been compiled with optimization flags.
#



# ---------------------------- Tests -----------------------------------

COMPILATION_FLAGS='-g -O'
. normal_compile.sh

echo "No optimization errors"
