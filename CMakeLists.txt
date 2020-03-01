cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")
set(LEVK_VERSION "0.0.0.1" CACHE STRING "Application Version" FORCE)
project(LittleEngineVk VERSION ${LEVK_VERSION})

# Options
option(LEVK_USE_GLFW "Use GLFW for Windowing" ON)

# Includes
include(.cmake/platform.cmake)
include(.cmake/utils.cmake)

# Header text
set(BUILD_CONFIG ${CMAKE_BUILD_TYPE})
if(CMAKE_CONFIGURATION_TYPES)
	set(BUILD_CONFIG "Multi-config")
endif()
message(STATUS "[${CMAKE_SYSTEM_PROCESSOR}] [${PLATFORM}] [${CMAKE_GENERATOR}] [${CMAKE_CXX_COMPILER_ID}] [${BUILD_CONFIG}]")
if(CI_BUILD)
	message(STATUS "== CI Build ==")
endif()

# Git operations
set(GIT_COMMIT_HASH "[unknown]")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
	get_git_commit_hash(GIT_COMMIT_HASH)
	option(LEVK_GIT_UPDATE_SUBMODULES "Init/Update submodules during generation" ON)
	if(LEVK_GIT_UPDATE_SUBMODULES)
		update_git_submodules(1)
	endif()
endif()

# Constants
set(LEVK_GIT_COMMIT_HASH ${GIT_COMMIT_HASH} CACHE STRING "Commit hash" FORCE)
set(EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external" CACHE INTERNAL "External dependencies root" FORCE)

# External
add_subdirectory(external/physfs)
add_subdirectory(external/fmt)
add_subdirectory(external/glm)
if(LEVK_USE_GLFW)
	add_subdirectory(external/glfw)
endif()

# Libraries
add_subdirectory(libs/core)

# levk
add_subdirectory(src)
