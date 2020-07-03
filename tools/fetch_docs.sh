#!/bin/bash

# This script clones/pulls docs from https://github.com/karnkaul/levk-docs
# Requirements:
#  - git

origin=https://github.com/karnkaul/levk-docs
subdir=docs

clean=false
nofetch=false

for arg in "$@"; do
  [ "$arg" == "-c" ] && clean=true
  [ "$arg" == "-n" ] && nofetch=true
done

this_dir=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh
cd "$this_dir"

if [ "$clean" == "true" ]; then
  rm -rf $subdir
  echo "-- Deleted $subdir --"
fi
if [ "$nofetch" == "false" ]; then
  if [ ! -d $subdir ]; then
    git clone $origin $subdir
    echo "== Cloned $origin into $subdir =="
  else
    cd "$subdir"
    git pull
    cd "$this_dir"
    echo "== Pulled from $origin into $subdir =="
  fi
fi

exit
