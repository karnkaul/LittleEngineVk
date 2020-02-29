#!/bin/bash

# This script runs clang-format on all source files
# Requirements:
#  - clang-format

EXCLUDE=*external/*
BUILD=*out/*
PATTERN0="*.hpp"
PATTERN1="*.cpp"
RUN=FALSE
[ "$1" == "-r" ] && RUN=TRUE

THISDIR=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh
cd "$THISDIR"

FILES=$(find . -not -path "$EXCLUDE" -not -path "$BUILD" \( -name $PATTERN0 -o -name $PATTERN1 \))
COUNT=$(echo -e "$FILES" | wc -l)
echo -e "\nFiles:\n\n$FILES\n"

if [ "$RUN" == "TRUE" ]; then
	$CLANG_FORMAT -i $FILES
	echo "== Ran clang-format on $COUNT source files"
else
	echo "-- Pass '-r' to run clang-format on $COUNT source files"
fi

exit
