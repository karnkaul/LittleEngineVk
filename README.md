# Little Engine Vulkan

An attempt to write a simple 3D game engine with a (mostly hard-coded) Vulkan renderer.

[Documentation](https://karnkaul.github.io/levk-docs) is located here (WIP).

[![Build status](https://ci.appveyor.com/api/projects/status/km8h75k4a1695umo/branch/main?svg=true)](https://ci.appveyor.com/project/karnkaul/littleenginevk/branch/main)

### Usage
- Clone this repo (manually initialising git submodules is not required, it will be done by CMake script)
- Use CMake and a generator of your choice
```
# Examples
cmake -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ . -B out/Debug
# OR
cmake . -B out/Debug
ccmake out/Debug
```
- Adjust CMake variables as needed: all variables pertaining to this project are prefixed with `LEVK`
- Build engine / demo / tests
```
cmake --build out/Debug
```
- Debug/Run desired executables / Link `levk-engine` to custom application

[Original repository](https://github.com/karnkaul/LittleEngineVk)

[LICENCE](LICENSE)
