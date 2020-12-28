#!/bin/bash

# This script fetches Android Vulkan validation layer binaries 
# from the specified GitHub release
# and stores them to lib/

lib_name=libVkLayer_khronos_validation.so
lib_dirs="arm64-v8a armeabi-v7a x86 x86_64"

version=1.2.162.0
archive_name=android-binaries-$version.tar.gz
url=https://github.com/KhronosGroup/Vulkan-ValidationLayers/releases/download/sdk-$version/$archive_name

[ -d demo/android/app/src ] && cd demo/android/app/src
[ -d android/app/src ] && cd android/app/src
[ -d app/src/main ] && cd app/src
[ -f src/main/AndroidManifest.xml ] && cd src

if [ ! -f main/AndroidManifest.xml ]; then
  echo -e "Unknown pwd"
  exit 1
fi

[ ! -d lib ] && mkdir lib
all_present=true
for dir in $lib_dirs; do
  if [ ! -f lib/$dir/$lib_name ]; then
    all_present=false
    break
  fi
done

if [ $all_present == false ]; then
  if [ ! -f lib/$archive_name ]; then
    echo -e "\n - Downloading Android validation layers...\n\n"
    curl -L $url > lib/$archive_name || exit 1
  fi
  echo -e "\n - Decompressing archive...\n"
  tar -xvf lib/$archive_name -C lib || exit 1
fi

if [ -f lib/$archive_name ]; then
  echo -e "\n - Removing artefacts...\n"
  rm lib/$archive_name
fi

echo -e "Android validation layers ready"

exit
