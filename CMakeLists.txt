cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")
set(LEVK_VERSION "0.0.0.1" CACHE STRING "Application Version" FORCE)
project(LittleEngineVk VERSION ${LEVK_VERSION})

if(NOT ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	message(FATAL_ERROR "Unsupported architecture, must be 64-bit")
endif()

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are not supported; choose a subdirectory like `out/*` or `build/*` for CMake output")
endif()

# Options
option(LEVK_USE_PCH "Generate pre-compiled header" ON)
option(LEVK_USE_GLFW "Use GLFW for Windowing" ON)
option(LEVK_BUILD_DEMO "Build demo" ON)
if(CMAKE_BUILD_TYPE STREQUAL Debug)
	option(LEVK_EDITOR "Enable Editor" ON)
	option(LEVK_BUILD_DOCS "Build documentation (Requires Doxygen)" OFF)
else()
	option(LEVK_EDITOR "Enable Editor" OFF)
	option(LEVK_BUILD_DOCS "Build documentation (Requires Doxygen)" ON)
endif()
if(NOT ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
	set(LEVK_USE_PCH OFF CACHE BOOL "" FORCE)
endif()
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	option(LEVK_BUILD_TESTS "Build Tests" ON)
else()
	option(LEVK_BUILD_TESTS "Build Tests" OFF)
endif()
if(LEVK_EDITOR)
	set(LEVK_USE_IMGUI ON CACHE BOOL "" FORCE)
endif()

execute_process(COMMAND doxygen --version RESULT_VARIABLE DOXYGEN_NOTFOUND OUTPUT_QUIET)
if(DOXYGEN_NOTFOUND)
	set(LEVK_BUILD_DOCS OFF CACHE BOOL "" FORCE)
else()
	set(LEVK_DOXYGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "" FORCE)
	set(LEVK_DOXYGEN_DOCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs" CACHE INTERNAL "" FORCE)
endif()

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
set(LEVK_GIT_COMMIT_HASH ${GIT_COMMIT_HASH} CACHE STRING "git commit hash" FORCE)
set(EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external" CACHE INTERNAL "External dependencies root" FORCE)

# External
add_subdirectory(external/physfs)
add_subdirectory(external/fmt)
add_subdirectory(external/glm)
if(LEVK_USE_GLFW)
	set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
	add_subdirectory(external/glfw)
endif()
add_subdirectory(external/vma)
add_subdirectory(external/stb)
add_subdirectory(external/tinyobjloader)
if(LEVK_USE_IMGUI)
	add_subdirectory(external/imgui)
endif()

# Interface
set(COMPILE_DEFS
	_UNICODE
	$<$<NOT:$<CONFIG:Debug>>:
		NDEBUG
		LEVK_RELEASE
	>
	$<$<CONFIG:Debug>:
		LEVK_DEBUG
	>
	$<$<BOOL:${MSVC_RUNTIME}>:WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS>
	# LEVK Flags
	$<$<BOOL:${LEVK_USE_GLFW}>:LEVK_USE_GLFW>
	$<$<BOOL:${LEVK_USE_IMGUI}>:LEVK_USE_IMGUI>
	$<$<BOOL:${LEVK_EDITOR}>:LEVK_EDITOR>
)
set(CLANG_COMMON -Wconversion -Wunreachable-code -Wdeprecated-declarations -Wtype-limits -Wunused)
if(LINUX_GCC OR LINUX_CLANG OR WIN64_GCC OR WIN64_CLANG)
	set(COMPILE_OPTS
		-Wextra
		-Werror=return-type
		$<$<NOT:$<CONFIG:Debug>>:-Werror>
		$<$<NOT:$<BOOL:${WIN64_CLANG}>>:-fexceptions>
		$<$<BOOL:${WIN64_CLANG}>:/W4>
		$<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${LINUX_CLANG}>>:-Wall>
		$<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${WIN64_GCC}>,$<BOOL:${WIN64_CLANG}>>:-utf-8>
		$<$<OR:$<BOOL:${LINUX_CLANG}>,$<BOOL:${WIN64_CLANG}>>:${CLANG_COMMON}>
	)
elseif(WIN64_MSBUILD)
	set(COMPILE_OPTS
		$<$<NOT:$<CONFIG:Debug>>:
			/O2
			/Oi
			/Ot
		>
		/MP
	)
endif()
if(PLATFORM STREQUAL "Linux")
	list(APPEND COMPILE_OPTS -fPIC)
	set(LINK_OPTS
		-no-pie         # Build as application
		-Wl,-z,origin   # Allow $ORIGIN in RUNPATH
	)
elseif(PLATFORM STREQUAL "Win64" AND NOT WIN64_GCC)
	set(LINK_OPTS
		/ENTRY:mainCRTStartup
		$<$<CONFIG:Debug>:
			/SUBSYSTEM:CONSOLE
			/OPT:NOREF
			/DEBUG:FULL
		>
		$<$<NOT:$<CONFIG:Debug>>:
			/SUBSYSTEM:WINDOWS
			/OPT:REF
			/OPT:ICF
			/INCREMENTAL:NO
		>
		$<$<CONFIG:RelWithDebinfo>:
			/DEBUG:FULL
		>
	)
endif()
add_library(levk-interface INTERFACE)
target_compile_definitions(levk-interface INTERFACE ${COMPILE_DEFS})
target_compile_options(levk-interface INTERFACE ${COMPILE_OPTS})
target_link_options(levk-interface INTERFACE ${LINK_OPTS})

# Libraries
add_subdirectory(libs/core)
add_subdirectory(src)

# Demo
if(LEVK_BUILD_DEMO)
	add_subdirectory(demo)
endif()

# Tests
enable_testing()

if(LEVK_BUILD_TESTS)
	include(CTest)
	enable_testing()
	add_subdirectory(src/tests)
endif()
