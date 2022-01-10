cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)	# don't let OPTION erase above non-cache variables
set(CMAKE_POLICY_DEFAULT_CMP0076 NEW)	# relative target_sources
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")
set(CMAKE_CXX_EXTENSIONS OFF CACHE INTERNAL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(target_name levk-engine)
set(LEVK_VERSION 0.0.9.0 CACHE STRING "Application Version" FORCE)
project(LittleEngineVk VERSION ${LEVK_VERSION})

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

if(${LEVK_VERSION} VERSION_LESS 1.0)
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
add_subdirectory(ext/dyvk)

# external
set(PHYSFS_BUILD_STATIC ON)
set(PHYSFS_BUILD_SHARED OFF)
set(PHYSFS_BUILD_DOCS OFF)
set(PHYSFS_ARCHIVE_GRP OFF)
set(PHYSFS_ARCHIVE_HOG OFF)
set(PHYSFS_ARCHIVE_ISO9660 OFF)
set(PHYSFS_ARCHIVE_MVL OFF)
set(PHYSFS_ARCHIVE_QPAK OFF)
set(PHYSFS_ARCHIVE_SLB OFF)
set(PHYSFS_ARCHIVE_VDF OFF)
set(PHYSFS_ARCHIVE_WAD OFF)
set(PHYSFS_ARCHIVE_ZIP ON)
set(PHYSFS_BUILD_TEST OFF)
add_subdirectory(ext/physfs)
target_include_directories(physfs-static SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext/physfs/src>)
add_subdirectory(ext/glm)
target_compile_options(glm INTERFACE 
  $<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${WINDOWS_GCC}>>:-Wno-volatile> 
  $<$<OR:$<BOOL:${LINUX_CLANG}>,$<BOOL:${WINDOWS_CLANG}>>:-Wno-deprecated-volatile>
)
if(LEVK_USE_GLFW)
  set(GLFW_VULKAN_STATIC OFF)
  set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
  set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
  add_subdirectory(ext/glfw)
  if(WINDOWS_CLANG)
    target_compile_options(glfw PRIVATE -w)
  endif()
endif()
add_subdirectory(ext/vma)
add_subdirectory(ext/stb)
add_subdirectory(ext/tinyobjloader)
if(LEVK_USE_IMGUI)
  add_subdirectory(ext/dear_imgui)
endif()
set(SPIRV_CROSS_CLI OFF)
set(SPIRV_CROSS_ENABLE_CPP OFF)
set(SPIRV_CROSS_ENABLE_C_API OFF)
set(SPIRV_CROSS_ENABLE_GLSL ON)
set(SPIRV_CROSS_ENABLE_HLSL OFF)
set(SPIRV_CROSS_ENABLE_MSL OFF)
set(SPIRV_CROSS_ENABLE_REFLECT OFF)
set(SPIRV_CROSS_ENABLE_TESTS OFF)
set(SPIRV_CROSS_ENABLE_UTIL OFF)
set(SPIRV_CROSS_SHARED OFF)
set(SPIRV_CROSS_STATIC ON)
add_subdirectory(ext/spirv_cross)
if(LINUX_CLANG OR LINUX_GCC OR WINDOWS_CLANG OR WINDOWS_GCC)
  target_compile_options(spirv-cross-core PRIVATE -w)
  target_compile_options(spirv-cross-glsl PRIVATE -w)
  target_include_directories(spirv-cross-glsl SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext/spirv_cross>)
elseif(PLATFORM STREQUAL Win64)
  target_compile_definitions(spirv-cross-core PUBLIC _CRT_SECURE_NO_WARNINGS NO_MIN_MAX)
endif()
add_subdirectory(ext/freetype)

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
  GIT_TAG a6c5f32
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
add_subdirectory(libs/window)
add_subdirectory(libs/graphics)
add_subdirectory(libs/levk_imgui)
# export clap targets for tools projects to find
export(EXPORT clap-targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/libs/clap/clap-targets.cmake"
  NAMESPACE clap::
)

# engine
set(levk_generated "${CMAKE_CURRENT_BINARY_DIR}/src/generated/build_version.hpp")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/build_version.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/generated/build_version.hpp")
file(GLOB_RECURSE levk_includes CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.?pp")
file(GLOB_RECURSE levk_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${levk_includes} ${levk_sources})
source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${levk_generated})
add_library(${target_name} ${LEVK_LIBRARY_TYPE} ${levk_includes} ${levk_sources} ${levk_generated})
add_library(levk::engine ALIAS ${target_name})
target_compile_definitions(${target_name} PUBLIC $<$<BOOL:${LEVK_EDITOR}>:LEVK_EDITOR>)
target_include_directories(${target_name} SYSTEM
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/src/generated"
)
target_include_directories(${target_name}
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/src/generated"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(${target_name}
  PUBLIC
    levk::imgui
    levk::graphics
    levk::window
    levk::core
    djson::djson
    dtasks::dtasks
    clap::clap
    levk::interface
    dens::dens

  PRIVATE
    levk::compile-options
    stb-image
    tinyobjloader
)
if(LEVK_USE_PCH)
  target_precompile_headers(${target_name} REUSE_FROM levk-graphics)
endif()

# demo
if(LEVK_BUILD_DEMO)
  add_subdirectory(demo)
endif()

# tests
if(LEVK_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
