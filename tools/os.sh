#!/bin/bash

# This script sets some env vars based on OS, shell, etc.
# Note: script is meant to be sourced, not called; it does not exit

if [[ "$OSTYPE" == "darwin"* ]]; then
	is_macosx=true
elif [[ "$OSTYPE" == "linux-gnu" ]]; then
	[ ! -z "$(grep Microsoft /proc/version)" ] && is_wsl=true
	if [ -z "$is_wsl" ]; then
		is_linux=true
	else
		is_win64=true
		run_batch="cmd.exe /c "
	fi
elif [[ "$OSTYPE" == "msys" ]]; then
	is_mingw=true
	is_win64=true
	run_batch="./"
elif [[ "$OSTYPE" == "cygwin" ]]; then
	is_cygwin=true
	is_win64=true
	run_batch="./"
fi

if [ "$is_win64" == "true" ]; then
	[ ! -z  "$(echo $INCLUDE | grep Microsoft)" ] && is_vcvars_set=true
	[ ! -z  "$(echo $PATH | grep 'Windows Kits')" ] && is_vcvars_set=true
fi

MAKE=make
if [ "$is_win64" == "true" ]; then
	ext=.exe
	MAKE=mingw32-$MAKE$ext
else
	is_symlinks=true
fi

if [ "$1" == "-s" ] || [ "$1" == "--status" ]; then
	echo -e "is_macosx\t: $is_macosx"
	echo -e "is_linux\t: $is_linux"
	echo -e "is_win64\t: $is_win64"
	echo -e "is_mingw\t: $is_mingw"
	echo -e "is_cygwin\t: $is_cygwin"
	echo -e "is_wsl\t\t: $is_wsl"
	echo -e "is_symlinks\t: $is_symlinks"
	echo -e "is_vcvars_set\t: $is_vcvars_set"
fi
