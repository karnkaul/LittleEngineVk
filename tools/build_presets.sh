#!/bin/bash

declare -a presets=(levk-db levk-rl levk-edrw levk-rw)

cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh

cd ..

for preset in "${presets[@]}"; do
  if [ "$1" == "--configure" ] || [ "$1" == "-c" ]; then
    cmake . --preset=$preset
  fi
  cmake --build --preset $preset
done
