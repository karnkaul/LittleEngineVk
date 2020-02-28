#!/bin/bash
test $DEBUG && set -x

## This script removes existing CMake (usually 3.12 on Ubuntu) and installs the latest:
##   - cmake
##   - ninja-build
##   - clang
##   - xorg

ENV="cmake g++-8 ninja-build clang xorg-dev"

# Get latest keys for clang and cmake
# cmake 3.15
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' -y

# Purge all existing cmake/clang installations
sudo apt-get purge {cmake,clang,lld}
sudo rm -rf /usr/{bin/cmake*,share/cmake*,local/bin/cmake*,local/cmake*}
sudo rm -rf /usr/{bin/clang*,share/clang*,local/bin/clang*}

# Install dependencies
sudo apt-get update
sudo apt-get install -y $ENV

exit
