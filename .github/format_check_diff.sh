#!/bin/bash

[[ ! $(git --version) ]] && exit 1

output=$(git diff)

if [[ "$output" != "" ]]; then
  echo -e "One or more source files are not formatted!\n\n$output\n"
  echo -e "Using $(clang-format --version)\n"
  exit 1
fi

echo "All source files are formatted"
exit
