#!/bin/bash

script_path=${0%/*}
project_root="$script_path/.."
commit=false
publish=false

while getopts "cp" arg; do
  case $arg in
    c) commit=true
    ;;
    p) publish=true
    ;;
  esac
done

if [[ ! $(doxygen --version) ]]; then
  echo "ERROR: doxygen not found"
  exit 1
fi

if [[ "$0" != "$project_root" ]] && [[ "$project_root" != "" ]]; then
  cd "$project_root" || exit 1
  echo "-- Changed pwd to $(pwd)"
fi

if [[ ! -d "out" ]]; then
  echo "-- Creating out"
  mkdir -p out
fi

cd out

if [[ ! -d "docs/.git" ]]; then
  echo "-- Cloning bave-docs"
  git clone https://github.com/karnkaul/bave-docs docs
else
  echo "-- Checking out main"
  cd docs
  git switch main > /dev/null
  git fetch && git reset --hard origin/main > /dev/null
fi

if [[ -d docs ]]; then
  echo "-- Removing old docs"
  rm -rf docs
fi

echo "-- Building docs"
cd ../..
doxygen -q
cd out/docs

if [[ ! -f .nojekyll ]]; then
  echo "-- Creating .nojekyll"
  touch .nojekyll
fi

if [[ $commit == true ]]; then
  echo "-- Creating commit"
  git add .
  git commit -m "[Automated] Build docs."

  if [[ $publish == true ]]; then
    echo "-- Pushing to origin"
    git push --force-with-lease
  fi
fi
