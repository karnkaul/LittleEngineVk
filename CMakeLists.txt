cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

set(build_version 0.1.0)
project(levk VERSION ${build_version})

option(LEVK_USE_FREETYPE "Build and link to Freetype (for text rendering)" ON)
option(LEVK_BUILD_IMPORTER "Build levk importer" ON)
option(LEVK_BUILD_EDITOR "Build levk editor" ${PROJECT_IS_TOP_LEVEL})

add_subdirectory(ext)
add_subdirectory(lib)

if(LEVK_BUILD_EDITOR)
  if(NOT LEVK_BUILD_IMPORTER)
    message(FATAL_ERROR "levk: Cannot build editor without importer")
  endif()

  add_subdirectory(editor)
endif()
