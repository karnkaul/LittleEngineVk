cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(spaced-engine VERSION 0.1.0)

option(SPACED_PCH "Use PCH for spaced" ON)
option(SPACED_IMPORTER "Build spaced-importer" ON)
option(SPACED_USE_FREETYPE "Use Freetype as the font backend" ON)

add_library(spaced-compile-options INTERFACE)
add_library(spaced::spaced-compile-options ALIAS spaced-compile-options)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(spaced-compile-options INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wunused -Werror=return-type
  )
endif()

add_subdirectory(ext)
add_subdirectory(engine)

if(SPACED_IMPORTER)
  add_subdirectory(importer)
endif()
