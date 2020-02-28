# C++17 CMake Project Template (Executable)

Sets up a minimalist C++17 CMake executable project with a few basic add-ons.

### Features
- Editor config support (use `.editorconfig.template` as an example)
- CMake tools:
    - `.cmake/platform.cmake` : sets cache variables for Windows/Linux platforms/compilers/etc (stateful)
    - `.cmake/utils.cmake` : provides various utility functions (stateless)
- Shell tools:
    - `tools/os.sh` : (import script) sets up environment for other scripts to run on (including WSL with exe binaries)
    - `tools/header-check.sh` : checks headers for `#pragma once` directive (after comments)
    - `tools/clang-format.sh` : runs clang-format on all source files
- Travis CI integration (Builds only, no tests):
    - `.travis/install.sh` : purges existing CMake, clang, etc, and installs C++17 compatible versions
    - `.travis/script.sh` : builds the project (multiple configurations supported)

[Original repository](https://github.com/karnkaul/cpp-template)

[LICENCE](LICENSE)
