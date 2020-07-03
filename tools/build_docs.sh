#!/bin/bash

# This script builds docs using Doxygen and pushes the docs subtree to a git branch
# Requirements:
#  - doxygen

doxyfile=Doxyfile
docs=docs
remote=origin
branch=master

push=false
nobuild=false
clean=false
force_push=false

for arg in "$@"; do
  [ "$arg" == "-p" ] && push=true
  [ "$arg" == "-c" ] && clean=true
  [ "$arg" == "-n" ] && nobuild=true
  [ "$arg" == "-f" ] && force_push=true
done

this_dir=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh

cd "$this_dir/$docs"

if [ ! -f ./$doxyfile ]; then
  echo "$doxyfile not found in $(pwd)!"
  exit 1
fi

if [ "$clean" == "true" ]; then
  rm -rf *.html *.css *.js *.png *.jpg
  echo "-- Cleaned ./$docs --"
fi

if [ "$nobuild" == "false" ]; then
  doxygen $doxyfile
  echo "== Built docs using ./$docs/$doxyfile =="
fi

if [ "$push" == "true" ]; then
  [[ "$force_push" == "true" ]] && f=--force-with-lease
  git checkout $branch
  git fetch
  git add .
  git commit -m "[Automated] Update docs"
  git push $f
  echo "== Pushed ./$docs to $remote/$branch =="
fi

cd "$this_dir"

exit
