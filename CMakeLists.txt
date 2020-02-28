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

# Executable
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.*pp"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
set(EXE_NAME ${PROJECT_NAME}-$<CONFIG>${EXE_SUFFIX})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${EXE_NAME})
add_target_compile_definitions(${PROJECT_NAME} LEVK)
add_target_compile_options(${PROJECT_NAME} PRIVATE)
add_target_link_options(${PROJECT_NAME})
if(W_MSBUILD)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
