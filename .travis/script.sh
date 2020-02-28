#!/bin/bash
PROJECT_NAME=cpp-template

test $DEBUG && set -x
set -e

echo -e "\n-- Tools:\n"
g++ --version;
clang --version; echo
cmake --version;
echo -e "\nninja: $(ninja --version)\n"

# Build cpp-template
build() 
{
	CONFIG=$1
	echo "== Building $PROJECT_NAME | $CONFIG..."
	[[ ! -d out/$CONFIG ]] && mkdir -p out/$CONFIG
	cmake -G Ninja $2 -B $3/$CONFIG -DCMAKE_BUILD_TYPE=$CONFIG -DGIT_UPDATE_SUBMODULES=0 -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCI_BUILD=1
	ninja -v -C out/$CONFIG $BUILD_FLAGS
}

# Build at least Release
[[ -z "$CONFIGS" ]] && CONFIGS=Release

# Build all configs
for CONFIG in $CONFIGS; do
	build $CONFIG $1 $2
done

exit
