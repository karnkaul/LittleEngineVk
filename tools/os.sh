#!/bin/bash

# This script sets some env vars based on OS, shell, etc.
# Note: script is meant to be sourced, not called; it does not exit

if [[ "$OSTYPE" == "darwin"* ]]; then
	IS_MACOSX=TRUE
elif [[ "$OSTYPE" == "linux-gnu" ]]; then
	[ ! -z "$(grep Microsoft /proc/version)" ] && IS_WSL=TRUE
	if [ -z "$IS_WSL" ]; then
		IS_LINUX=TRUE
	else
		IS_WIN64=TRUE
		RUN_BATCH="cmd.exe /c "
	fi
elif [[ "$OSTYPE" == "msys" ]]; then
	IS_MINGW=TRUE
	IS_WIN64=TRUE
	RUN_BATCH="./"
elif [[ "$OSTYPE" == "cygwin" ]]; then
	IS_CYGWIN=TRUE
	IS_WIN64=TRUE
	RUN_BATCH="./"
fi

if [ "$IS_WIN64" == "TRUE" ]; then
	[ ! -z  "$(echo $INCLUDE | grep Microsoft)" ] && IS_VCVARS_SET=TRUE
	[ ! -z  "$(echo $PATH | grep 'Windows Kits')" ] && IS_VCVARS_SET=TRUE
fi

MAKE=make
if [ "$IS_WIN64" == "TRUE" ]; then
	EXT=.exe
	MAKE=mingw32-$MAKE$EXT
else
	IS_SYMLINKS=TRUE
fi
CMAKE=cmake$EXT
NINJA=ninja$EXT
CLANG_FORMAT=clang-format$EXT

if [ "$1" == "-s" ] || [ "$1" == "--status" ]; then
	echo -e "IS_MACOSX\t: $IS_MACOSX"
	echo -e "IS_LINUX\t: $IS_LINUX"
	echo -e "IS_WIN64\t: $IS_WIN64"
	echo -e "IS_MINGW\t: $IS_MINGW"
	echo -e "IS_CYGWIN\t: $IS_CYGWIN"
	echo -e "IS_WSL\t\t: $IS_WSL"
	echo -e "IS_SYMLINKS\t: $IS_SYMLINKS"
	echo -e "IS_VCVARS_SET\t: $IS_VCVARS_SET"
fi
