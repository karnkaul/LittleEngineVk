#!/bin/bash

# This script runs clang-format on all source files
# Requirements:
#  - clang-format

CLANG_FORMAT=clang-format$ext

EXCLUDE0=*/ext/*
EXCLUDE1=*/.cxx/*
BUILD=*out/*
PATTERN0="*.hpp"
PATTERN1="*.cpp"
RUN=FALSE
[ "$1" == "-r" ] && RUN=TRUE

THISDIR=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh
cd "$THISDIR"

FILES=$(find . -not -path "$EXCLUDE0" -not -path "$BUILD" -not -path "$EXCLUDE1" \( -name $PATTERN0 -o -name $PATTERN1 \))
COUNT=$(echo -e "$FILES" | wc -l)
echo -e "\nFiles:\n\n$FILES\n"
echo -e "\n$($CLANG_FORMAT --version)\n"

if [ "$RUN" == "TRUE" ]; then
	$CLANG_FORMAT -i $FILES
	echo "== Ran clang-format on $COUNT source files"
else
	echo "-- Pass '-r' to run clang-format on $COUNT source files"
fi

exit
