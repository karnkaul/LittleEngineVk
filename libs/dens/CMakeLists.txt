cmake_minimum_required(VERSION 3.14)
enable_language(CXX)

set(DENS_VERSION "0.1" CACHE STRING "dens version" FORCE)
project(dens VERSION ${DENS_VERSION})

set(is_root_project OFF)	# indicate if this is the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(is_root_project ON)
  message(STATUS "${PROJECT_NAME} is top level")
endif()

option(DENS_BUILD_TESTS "Build dens tests" ${is_root_project})

add_library(dens INTERFACE)
add_library(dens::dens ALIAS dens)
target_compile_features(dens INTERFACE cxx_std_20)
target_include_directories(dens SYSTEM INTERFACE include)
add_subdirectory(include/dens)

get_target_property(sources dens SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})
