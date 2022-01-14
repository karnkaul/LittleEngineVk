cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)	# don't let OPTION erase above non-cache variables
set(CMAKE_POLICY_DEFAULT_CMP0076 NEW)	# relative target_sources
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")
set(CMAKE_CXX_EXTENSIONS OFF CACHE INTERNAL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# build version
include(cmake/build_version.cmake)

# project parameters
project(LittleEngineVk VERSION ${build_version})

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not supported; choose a subdirectory like `out/*` or `build/*` for CMake output")
endif()

set(is_root_project OFF)	# indicate if this is the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(is_root_project ON)
  message(STATUS "${PROJECT_NAME} is top level")
  set(LEVK_CONFIGURE_CI_SCRIPTS ON)
endif()

# includes
include(CheckPIESupported)
check_pie_supported()
include(FetchContent)
include(cmake/platform.cmake)
include(cmake/utils.cmake)

if(NOT ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
  message(WARNING "Unsupported architecture, must be 64-bit")
endif()

if(${build_version} VERSION_LESS 1.0)
  set(LEVK_PRE_RELEASE ON)
else()
  set(LEVK_PRE_RELEASE OFF)
endif()

# options
option(LEVK_USE_PCH "Generate pre-compiled header" ON)
option(LEVK_USE_GLFW "Use GLFW for Windowing" ON)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  option(LEVK_EDITOR "Enable Editor" ON)
else()
  option(LEVK_EDITOR "Enable Editor" OFF)
endif()
if(NOT ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
  set(LEVK_USE_PCH OFF CACHE BOOL "" FORCE)
endif()
option(LEVK_BUILD_TESTS "Build Tests" ${is_root_project})
option(LEVK_BUILD_DEMO "Build demo" ${is_root_project})
option(LEVK_INSTALL "Install levk and dependencies (WIP)" ${is_root_project})
if(LEVK_INSTALL)
  set(KTL_INSTALL ON)
  set(DTASKS_INSTALL ON)
  set(DENS_INSTALL ON)
  set(DYVK_INSTALL ON)
  set(DJSON_INSTALL ON)
  set(DLOG_INSTALL ON)
  set(CLAP_INSTALL ON)
  if(NOT KTL_INSTALL OR NOT DTASKS_INSTALL OR NOT DENS_INSTALL OR NOT DYVK_INSTALL OR NOT DJSON_INSTALL OR NOT DLOG_INSTALL)
    message(WARNING "Invalid INSTALL flags")
  endif()
endif()

if(LINUX_CLANG OR WINDOWS_CLANG)
  option(LEVK_ASAN "Enable ASAN" OFF)
  option(LEVK_UBSAN "Enable UBSAN" OFF)
endif()

# debug libstdc++
if(is_root_project AND GCC_RUNTIME AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT LEVK_ASAN AND NOT LEVK_UBSAN)
  add_definitions(-D_GLIBCXX_DEBUG)
endif()

if(LEVK_EDITOR)
  set(LEVK_USE_IMGUI ON CACHE BOOL "" FORCE)
  if(NOT LEVK_USE_GLFW OR NOT LEVK_USE_IMGUI)
    message(FATAL_ERROR "Cannot build Editor without LEVK_USE_GLFW and LEVK_USE_IMGUI!")
  endif()
endif()

# Git operations
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
  git_get_commit_hash(LEVK_GIT_COMMIT_HASH)	# store commit hash in LEVK_GIT_COMIT_HASH
  if(is_root_project)
    option(LEVK_GIT_UPDATE_SUBMODULES "Init/Update submodules during generation" ${is_root_project})
    if(LEVK_GIT_UPDATE_SUBMODULES)
      git_update_submodules(FATAL_ERROR)
    endif()
  endif()
endif()

# interface
add_subdirectory(cmake/interface)

# header text
set(build_config ${CMAKE_BUILD_TYPE})
if(CMAKE_CONFIGURATION_TYPES)
  set(build_config "Multi-config")
elseif(LEVK_ASAN OR LEVK_UBSAN)
  set(build_config "${CMAKE_BUILD_TYPE} ASAN/UBSAN")
endif()
message(STATUS "[${CMAKE_SYSTEM_PROCESSOR}] [${PLATFORM}] [${CMAKE_GENERATOR}] [${CMAKE_CXX_COMPILER_ID}] [${build_config}]")
if(CI_BUILD)
  message(STATUS "== CI Build ==")
endif()

# dyvk
add_subdirectory(libs/dyvk)

# ext
add_subdirectory(ext)

# engine
set(LEVK_LIBRARY_TYPE STATIC CACHE STRING "levk library type (STATIC/SHARED)")

if(PLATFORM STREQUAL Win64 AND NOT LEVK_LIBRARY_TYPE STREQUAL STATIC)
  message(FATAL_ERROR "`LEVK_LIBRARY_TYPE` must be `STATIC` on Windows; DLLs are not supported!")
endif()
if(LEVK_USE_PCH)
  if(NOT LEVK_LIBRARY_TYPE STREQUAL STATIC)
    message(WARNING "Cannot use PCH with a shared library!")
    set(LEVK_USE_PCH OFF CACHE BOOL "" FORCE)
  endif()
  if(LEVK_ASAN OR LEVK_UBSAN)
    message(WARNING "PCH with ASAN/UBSAN is unreliable; disabling")
    set(LEVK_USE_PCH OFF CACHE BOOL "" FORCE)
  endif()
endif()

# ktl
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
  ktl
  GIT_REPOSITORY https://github.com/karnkaul/ktl
  GIT_TAG 5278a75
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/ktl"
)
FetchContent_MakeAvailable(ktl)
FetchContent_GetProperties(ktl)
if(NOT ktl_POPULATED)
  message(FATAL_ERROR "Failed to populate ktl")
else()
  message(STATUS "ktl populated at: ${ktl_SOURCE_DIR}")
endif()

# dtest
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
  dtest
  GIT_REPOSITORY https://github.com/karnkaul/dtest
  GIT_TAG 0ca8c64
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/dtest"
)
FetchContent_MakeAvailable(dtest)
FetchContent_GetProperties(dtest)
if(NOT dtest_POPULATED)
  message(FATAL_ERROR "Failed to populate dtest")
else()
  message(STATUS "dtest populated at: ${dtest_SOURCE_DIR}")
endif()

# libraries
add_subdirectory(libs/dlog)
add_subdirectory(libs/djson)
add_subdirectory(libs/dtasks)
set(DENS_BUILD_TESTS ON)
add_subdirectory(libs/dens)
add_subdirectory(libs/clap)
add_subdirectory(libs/core)
add_subdirectory(levk/window)
add_subdirectory(levk/graphics)
# export clap targets for tools projects to find
export(EXPORT clap-targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/libs/clap/clap-targets.cmake"
  NAMESPACE clap::
)

# engine
add_subdirectory(levk/engine)

# demo
if(LEVK_BUILD_DEMO)
  add_subdirectory(demo)
endif()

# tests
if(LEVK_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
