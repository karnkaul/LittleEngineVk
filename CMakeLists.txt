cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")
set(CMAKE_CXX_EXTENSIONS OFF CACHE INTERNAL "" FORCE)

set(TARGET_NAME levk-engine)
set(LEVK_VERSION 0.0.1.0 CACHE STRING "Application Version" FORCE)
project(LittleEngineVk VERSION ${LEVK_VERSION})

if(NOT ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	message(FATAL_ERROR "Unsupported architecture, must be 64-bit")
endif()

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are not supported; choose a subdirectory like `out/*` or `build/*` for CMake output")
endif()

# Includes
include(cmake/platform.cmake)
include(cmake/utils.cmake)

# Options
option(LEVK_USE_PCH "Generate pre-compiled header" ON)
option(LEVK_USE_GLFW "Use GLFW for Windowing" ON)
option(LEVK_BUILD_DEMO "Build demo" ON)
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	option(LEVK_EDITOR "Enable Editor" ON)
else()
	option(LEVK_EDITOR "Enable Editor" OFF)
endif()
if(NOT ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
	set(LEVK_USE_PCH OFF CACHE BOOL "" FORCE)
endif()
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	option(LEVK_BUILD_TESTS "Build Tests" ON)
else()
	option(LEVK_BUILD_TESTS "Build Tests" OFF)
endif()
if(LEVK_EDITOR)
	if(NOT LEVK_USE_GLFW)
		message(FATAL_ERROR "Cannot build Editor without GLFW and ImGui!")
	endif()
	set(LEVK_USE_IMGUI ON CACHE BOOL "" FORCE)
endif()

# Git operations
set(GIT_COMMIT_HASH "[unknown]")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
	get_git_commit_hash(GIT_COMMIT_HASH)
	option(LEVK_GIT_UPDATE_SUBMODULES "Init/Update submodules during generation" ON)
	if(LEVK_GIT_UPDATE_SUBMODULES)
		update_git_submodules(1)
	endif()
endif()

# Constants
set(LEVK_GIT_COMMIT_HASH ${GIT_COMMIT_HASH} CACHE STRING "git commit hash" FORCE)
set(EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external" CACHE INTERNAL "External dependencies root" FORCE)

# Interface
include(cmake/interface.cmake)

# Header text
set(BUILD_CONFIG ${CMAKE_BUILD_TYPE})
if(CMAKE_CONFIGURATION_TYPES)
	set(BUILD_CONFIG "Multi-config")
endif()
message(STATUS "[${CMAKE_SYSTEM_PROCESSOR}] [${PLATFORM}] [${CMAKE_GENERATOR}] [${CMAKE_CXX_COMPILER_ID}] [${BUILD_CONFIG}]")
if(CI_BUILD)
	message(STATUS "== CI Build ==")
endif()

# External
add_subdirectory(external/physfs)
add_subdirectory(external/fmt)
target_compile_features(fmt PRIVATE cxx_std_17)
add_subdirectory(external/glm)
if(LEVK_USE_GLFW)
	set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
	add_subdirectory(external/glfw)
	if(WIN64_CLANG AND LEVK_USE_GLFW)
		target_compile_options(glfw PRIVATE -Wno-everything)
	endif()
endif()
add_subdirectory(external/vma)
add_subdirectory(external/stb)
add_subdirectory(external/tinyobjloader)
if(LEVK_USE_IMGUI)
	add_subdirectory(external/imgui)
endif()

# Libraries
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/.appveyor.yml.in" "${CMAKE_CURRENT_SOURCE_DIR}/.appveyor.yml")
add_subdirectory(libs/core)
add_subdirectory(libs/djson)

# Engine
set(LEVK_LIBRARY_TYPE STATIC CACHE STRING "levk library type (STATIC/SHARED)")

if(PLATFORM STREQUAL "Win64" AND NOT LEVK_LIBRARY_TYPE STREQUAL STATIC)
	message(FATAL_ERROR "`LEVK_LIBRARY_TYPE` must be `STATIC` on Windows; DLLs are not supported!")
endif()
if(NOT LEVK_LIBRARY_TYPE STREQUAL STATIC AND LEVK_USE_PCH)
	message(WARNING "Cannot use PCH with a shared library!")
	set(LEVK_USE_PCH OFF CACHE BOOL "" FORCE)
endif()

# Vulkan
find_package(Vulkan)
if(NOT Vulkan_FOUND)
	message(FATAL_ERROR 
"Vulkan not installed! LittleEngineVk requires Vulkan ICD loader, headers, and validation layers!
Note: for custom SDK locations, define `VULKAN_SDK` and source `vk-setup-env.sh` into this shell (and runtime environment).
Make sure to delete CMakeCache.txt before re-configuring!"
	)
else()
	message(STATUS "== Vulkan ==\n\tHeaders : ${Vulkan_INCLUDE_DIRS}\n\tLibraries : ${Vulkan_LIBRARIES}")
endif()

# VMA
target_link_libraries(vma PRIVATE ${Vulkan_LIBRARIES})
target_include_directories(vma PRIVATE ${Vulkan_INCLUDE_DIRS})

if(LEVK_USE_IMGUI)
	target_include_directories(imgui PRIVATE ${Vulkan_INCLUDE_DIRS})
endif()

# Library
set(LEVK_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/src/generated/build_version.hpp")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/build_version.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/generated/build_version.hpp")
file(GLOB_RECURSE LEVK_INCLUDES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.?pp")
file(GLOB_RECURSE LEVK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${LEVK_INCLUDES} ${LEVK_SOURCES})
source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${LEVK_GENERATED})
add_library(${TARGET_NAME} ${LEVK_LIBRARY_TYPE} ${LEVK_INCLUDES} ${LEVK_SOURCES} ${LEVK_GENERATED})
target_include_directories(${TARGET_NAME} PRIVATE 
	INTERFACE SYSTEM
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/src/generated"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/src/generated"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${Vulkan_INCLUDE_DIRS}"
)
target_link_libraries(${TARGET_NAME}
	PUBLIC
		levk-core
		djson
	PRIVATE
		levk-interface
		${Vulkan_LIBRARIES}
		$<$<STREQUAL:${PLATFORM},Linux>:X11>
		$<$<BOOL:${LEVK_USE_GLFW}>:glfw>
		$<$<BOOL:${LEVK_USE_IMGUI}>:imgui>
		vma
		stb-image
		tinyobjloader
)
if(LEVK_USE_PCH AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
	set(PRECOMPILE
		
	)
	target_precompile_headers(${TARGET_NAME} 
		PUBLIC
			<deque>
			<filesystem>
			<string>
			<unordered_map>
			<vector>
		PRIVATE
			<vk_mem_alloc.h>
			<stb/stb_image.h>
			<tinyobjloader/tiny_obj_loader.h>
	)
endif()

# Demo
if(LEVK_BUILD_DEMO)
	add_subdirectory(demo)
endif()

# Tests
enable_testing()

if(LEVK_BUILD_TESTS)
	include(CTest)
	enable_testing()
	add_subdirectory(tests)
endif()
