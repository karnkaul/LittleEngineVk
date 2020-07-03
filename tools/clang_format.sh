#!/bin/bash

# This script runs clang-format on all source files
# Requirements:
#  - clang-format

exclude=*external/*
build=*out/*
pattern0="*.hpp"
pattern1="*.cpp"
run=false
[ "$1" == "-r" ] && run=true

this_dir=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh
cd "$this_dir"

files=$(find . -not -path "$exclude" -not -path "$build" \( -name $pattern0 -o -name $pattern1 \))
count=$(echo -e "$files" | wc -l)
echo -e "\nfiles:\n\n$files\n"

if [ "$run" == "true" ]; then
	clang-format$ext -i $files
	echo "== Ran clang-format on $count source files"
else
	echo "-- Pass '-r' to run clang-format on $count source files"
fi

exit
