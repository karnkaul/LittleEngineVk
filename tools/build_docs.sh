#!/bin/bash

# This script builds docs using Doxygen and pushes the docs subtree to a git branch
# Requirements:
#  - doxygen

doxyfile=Doxyfile
docs=docs
remote=origin
branch=gh-pages

if [ -z ./$doxyfile ]; then
  echo "$doxyfile not found in $(pwd)!"
  exit 1
fi

deploy=false
[ "$1" == "-d" ] && deploy=true

this_dir=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh
cd "$this_dir"

doxygen Doxyfile
echo "== Built docs using ./$doxyfile to ./$docs =="

if [ "$deploy" == "true" ]; then
	git subtree push --prefix $docs $remote $branch
	echo "== Pushed ./$docs to $remote/$branch =="
fi

exit
