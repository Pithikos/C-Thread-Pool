#! /bin/bash

#
# This file has several tests to check for memory leaks.
# valgrind is used so make sure you have it installed
#

. funcs.sh


# ---------------------------- Tests -----------------------------------

function test_thread_free { #threads
	echo "Testing creation and destruction of threads(=$1)"
	compile src/no_work.c
	output=$(valgrind --leak-check=full --track-origins=yes ./test "$1" 2>&1 > /dev/null)
	heap_usage=$(echo "$output" | grep "total heap usage")
	allocs=$(extract_num "[0-9]* allocs" "$heap_usage")
	frees=$(extract_num "[0-9]* frees" "$heap_usage")
	if (( "$allocs" == 0 )); then
		err "Allocated 0 times. Something is wrong.." "$output"
	fi
	if (( "$allocs" == "$frees" )); then
		return
	fi
	err "Allocated $allocs times but freed only $frees" "$output"
	exit 1
}


function _test_thread_free_multi { #threads
	output=$(valgrind --leak-check=full --track-origins=yes ./test "$1" 2>&1 > /dev/null)
	heap_usage=$(echo "$output" | grep "total heap usage")
	allocs=$(extract_num "[0-9]* allocs" "$heap_usage")
	frees=$(extract_num "[0-9]* frees" "$heap_usage")
	if (( "$allocs" == 0 )); then
		err "Allocated 0 times. Something is wrong.." "$output"
		return 1
	fi
	if (( "$allocs" != "$frees" )); then
		err "Allocated $allocs times but freed only $frees" "$output"
		return 1
	fi
	#echo "Allocs: $allocs    Frees: $frees"
}


# This is the same with test_many_thread_allocs but multiplied
function test_thread_free_multi { #threads #times #nparallel
	echo "Testing multiple threads creation and destruction in pool(threads=$1 times=$2)"
	compile src/no_work.c
	pids=()
	nparallel="${3:-10}"

	# Run tests in p
	for (( i = 1; i <= "$2"; i++ )); do
		
		# Run test in background
		python -c "import sys; sys.stdout.write('$i/$2\r')"
		_test_thread_free_multi "$1" &
		pids+=($!)

		# Wait for 10 background jobs to finish
		if (( "$i" % 10 == 0 )); then
			for pid in ${pids[@]}; do
				wait $pid
				if (( $? != 0 )); then
					err "Test failed" "Test failed"
				fi
			done
			pids=()
		fi
	done
	echo
}



# Run tests
assure_installed_valgrind
test_thread_free 1
test_thread_free 2
test_thread_free 4
test_thread_free 8
test_thread_free 1
test_thread_free 20
test_thread_free_multi 4 20

# test_thread_free_multi 3 1000  # Takes way too long
test_thread_free_multi 3 200

# test_thread_free_multi 100 100  # Takes way too long
test_thread_free_multi 100 20 1


echo "No memory leaks"
