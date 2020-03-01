#!/bin/bash

# This script ensures all headers have #pragma once
# Requirements:
#  - clang-format

EXCLUDE=*external/*
BUILD=*out/*
PATTERN0="*.h*"
[ "$1" == "-r" ] && RUN=TRUE

THISDIR=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh
cd "$THISDIR"

FILES=$(find . -not -path "$EXCLUDE" -not -path "$BUILD" \( -name $PATTERN0 \))
COUNT=$(echo -e "$FILES" | wc -l)
let COUNT=$COUNT-1
echo -e "== Checking $COUNT headers for:\n\t[#pragma once]\n"

IDX=0
ERR=false
for FILE in ${FILES}; do
	while read -r LINE; do
		[[ "$LINE" == *"*/" ]] && COMMENTED=false
		[[ "$LINE" == "/*"* ]] && COMMENTED=true
		if [[ "$COMMENTED" != "true" ]]; then
			[[ -z "${LINE// }" || "$LINE" == *"*/" || "$LINE" == "//"* ]] && continue
			if [[ "$LINE" != "#pragma once" ]]; then
				let IDX=$IDX+1
				echo -e "  $IDX. ERROR! $FILE missing '#pragma once'!"
				ERR=true
				break
			fi
			break
		fi
	done < $FILE
done
if [[ $ERR == "false" ]]; then
	echo -e "  All OK\n"
fi

exit
