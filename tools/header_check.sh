#!/bin/bash

# This script ensures all headers have #pragma once
# Requirements:
#  - clang-format

exclude=*external/*
build=*out/*
pattern="*.h*"

this_dir=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh
cd "$this_dir"

files=$(find . -not -path "$exclude" -not -path "$build" \( -name $pattern \))
count=$(echo -e "$files" | wc -l)
let count=$count-1
echo -e "== Checking $count headers for:\n\t[#pragma once]\n"

idx=0
err=false
for file in ${files}; do
	while read -r line; do
		[[ "$line" == *"*/" ]] && commented=false
		[[ "$line" == "/*"* ]] && commented=true
		if [[ "$commented" != "true" ]]; then
			[[ -z "${line// }" || "$line" == *"*/" || "$line" == "//"* ]] && continue
			if [[ "$line" != "#pragma once" ]]; then
				let idx=$idx+1
				echo -e "  $idx. errOR! $file missing '#pragma once'!"
				err=true
				break
			fi
			break
		fi
	done < $file
done
if [[ $err == "false" ]]; then
	echo -e "  All OK\n"
fi

exit
