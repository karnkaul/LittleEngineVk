cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")
set(APP_VERSION "0.0.0.1" CACHE STRING "Application Version" FORCE)
project(LittleEngineVk VERSION ${APP_VERSION})

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
	option(GIT_UPDATE_SUBMODULES "Init/Update submodules during generation" ON)
	if(GIT_UPDATE_SUBMODULES)
		update_git_submodules(1)
	endif()
endif()

# Constants
set(GIT_COMMIT_HASH ${GIT_COMMIT_HASH} CACHE STRING "Commit hash" FORCE)
set(EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external" CACHE INTERNAL "External dependencies root" FORCE)

# External
# physfs
add_subdirectory(external/physfs)
# fmt
add_subdirectory(external/fmt)
# glm
add_subdirectory(external/glm)

# Libraries
# core
add_subdirectory(libs/core)

# Executable
# levk
add_subdirectory(src)
