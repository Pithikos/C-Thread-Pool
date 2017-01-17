#! /bin/bash

#
# This file should be included by other shell scripts that
# want to use any of the functions
#

function assure_installed_valgrind {
    valgrind --version &> /dev/null
    if (( $? != 0 )); then
		msg="Valgrind seems to not be installed."
		err "$msg" "$msg"
	fi
}


function needle { #needle #haystack
	python -c "import re; print(re.search(r'$1', '$2').group(0))"
	if (( $? != 0 )); then
		msg="Python script error"
		err "$msg" "$msg"
	fi
}


function extract_num { #needle with number #haystack
	string=$(needle "$1" "$2")
	needle "[0-9]*" "$string"
	if (( $? != 0 )); then
		msg="Python script error"
		err "$msg" "$msg"
	fi
}


function time_exec { #command ..
	realsecs=$(/usr/bin/time -f '%e' "$@" 2>&1 > /dev/null)
	echo "$realsecs"
}


function err { #string #log
	echo "------------------- ERROR ------------------------"
	echo "$1"
	echo "$2" >> error.log
	exit 1
}


function compile { #cfilepath
	gcc $COMPILATION_FLAGS "$1" ../thpool.c -D THPOOL_DEBUG -pthread -o test
}
