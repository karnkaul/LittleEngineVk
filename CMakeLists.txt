cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)	# Don't let OPTION erase above non-cache variables
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "")
set(CMAKE_CXX_EXTENSIONS OFF CACHE INTERNAL "" FORCE)

set(TARGET_NAME levk-engine)
set(LEVK_VERSION 0.0.9.0 CACHE STRING "Application Version" FORCE)
project(LittleEngineVk VERSION ${LEVK_VERSION})

if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds are not supported; choose a subdirectory like `out/*` or `build/*` for CMake output")
endif()

# Includes
include(FetchContent)
include(cmake/platform.cmake)
include(cmake/utils.cmake)

if(NOT PLATFORM STREQUAL Android AND NOT ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	message(WARNING "Unsupported architecture, must be 64-bit")
endif()

if(${LEVK_VERSION} VERSION_LESS 1.0)
	set(LEVK_PRE_RELEASE ON)
else()
	set(LEVK_PRE_RELEASE OFF)
endif()

# Options
option(LEVK_USE_PCH "Generate pre-compiled header" ON)
if(${PLATFORM} STREQUAL Android)
	option(LEVK_VULKAN_DYNAMIC "Load Vulkan dynamically" ON)
	set(LEVK_EDITOR OFF CACHE BOOL "" FORCE)
	set(LEVK_USE_GLFW OFF CACHE BOOL "" FORCE)
	set(LEVK_USE_IMGUI OFF CACHE BOOL "" FORCE)
else()
	option(LEVK_USE_GLFW "Use GLFW for Windowing" ON)
	option(LEVK_VULKAN_DYNAMIC "Load Vulkan dynamically" OFF)
	if("$CMAKE_BUILD_TYPE" STREQUAL "Debug")
		option(LEVK_EDITOR "Enable Editor" ON)
	else()
		option(LEVK_EDITOR "Enable Editor" OFF)
	endif()
endif()
if(NOT ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
	set(LEVK_USE_PCH OFF CACHE BOOL "" FORCE)
endif()
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	option(LEVK_BUILD_TESTS "Build Tests" ON)
	option(LEVK_BUILD_DEMO "Build demo" ON)
else()
	option(LEVK_BUILD_TESTS "Build Tests" OFF)
	option(LEVK_BUILD_DEMO "Build demo" OFF)
endif()
option(LEVK_COPY_CCJSON "Copy compile_commands.json to project root" OFF)

if(LINUX_CLANG OR WINDOWS_CLANG)
	option(LEVK_ASAN OFF)
endif()

if(PLATFORM STREQUAL Android)
	if(LEVK_EDITOR OR LEVK_USE_GLFW OR LEVK_USE_IMGUI)
		message(FATAL_ERROR "Cannot build Android with LEVK_EDITOR or LEVK_USE_GLFW or LEVK_USE_IMGUI!")
	endif()
	if(NOT LEVK_VULKAN_DYNAMIC)
		message(FATAL_ERROR "Cannot build Android without LEVK_VULKAN_DYNAMIC!")
	endif()
endif()
if(LEVK_EDITOR)
	set(LEVK_USE_IMGUI ON CACHE BOOL "" FORCE)
	if(NOT LEVK_USE_GLFW OR NOT LEVK_USE_IMGUI)
		message(FATAL_ERROR "Cannot build Editor without LEVK_USE_GLFW and LEVK_USE_IMGUI!")
	endif()
endif()
if(LEVK_USE_IMGUI AND LEVK_VULKAN_DYNAMIC)
	message(FATAL_ERROR "Cannot have both LEVK_USE_IMGUI and LEVK_VULKAN_DYNAMIC set!")
endif()

# Git operations
set(GIT_COMMIT_HASH "[unknown]")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
	get_git_commit_hash(GIT_COMMIT_HASH)
	if ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
		option(LEVK_GIT_UPDATE_SUBMODULES "Init/Update submodules during generation" ON)
	else()
		option(LEVK_GIT_UPDATE_SUBMODULES "Init/Update submodules during generation" OFF)
	endif()
	if(LEVK_GIT_UPDATE_SUBMODULES)
		update_git_submodules(1)
	endif()
endif()

# Constants
set(LEVK_GIT_COMMIT_HASH ${GIT_COMMIT_HASH} CACHE STRING "git commit hash" FORCE)

# Interface
include(cmake/interface.cmake)

# Header text
set(BUILD_CONFIG ${CMAKE_BUILD_TYPE})
if(CMAKE_CONFIGURATION_TYPES)
	set(BUILD_CONFIG "Multi-config")
elseif(LEVK_ASAN)
	set(BUILD_CONFIG "${CMAKE_BUILD_TYPE} ASAN")
endif()
message(STATUS "[${CMAKE_SYSTEM_PROCESSOR}] [${PLATFORM}] [${CMAKE_GENERATOR}] [${CMAKE_CXX_COMPILER_ID}] [${BUILD_CONFIG}]")
if(CI_BUILD)
	message(STATUS "== CI Build ==")
endif()

# Vulkan
add_subdirectory(ext/vulkan_hpp)

# External
add_subdirectory(ext/physfs)
add_subdirectory(ext/glm)
if(LEVK_USE_GLFW)
	set(GLFW_INSTALL OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
	add_subdirectory(ext/glfw)
	if(WINDOWS_CLANG)
		target_compile_options(glfw PRIVATE -Wno-everything)
	endif()
endif()
add_subdirectory(ext/vma)
add_subdirectory(ext/stb)
add_subdirectory(ext/tinyobjloader)
if(LEVK_USE_IMGUI)
	add_subdirectory(ext/imgui)
endif()

# Engine
set(LEVK_LIBRARY_TYPE STATIC CACHE STRING "levk library type (STATIC/SHARED)")

if(PLATFORM STREQUAL Win64 AND NOT LEVK_LIBRARY_TYPE STREQUAL STATIC)
	message(FATAL_ERROR "`LEVK_LIBRARY_TYPE` must be `STATIC` on Windows; DLLs are not supported!")
endif()
if(NOT LEVK_LIBRARY_TYPE STREQUAL STATIC AND LEVK_USE_PCH)
	message(WARNING "Cannot use PCH with a shared library!")
	set(LEVK_USE_PCH OFF CACHE BOOL "" FORCE)
endif()

# Libraries
if(NOT CI_BUILD)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/.appveyor.yml.in" "${CMAKE_CURRENT_SOURCE_DIR}/.appveyor.yml")
	# configure_file("${CMAKE_CURRENT_SOURCE_DIR}/.appveyor.yml.fallback.in" "${CMAKE_CURRENT_SOURCE_DIR}/.appveyor.yml")
endif()
add_subdirectory(libs/dlog)
add_subdirectory(libs/djson)
add_subdirectory(libs/dtasks)
add_subdirectory(libs/decf)
add_subdirectory(libs/clap)
add_subdirectory(libs/ktest)
add_subdirectory(libs/core)
add_subdirectory(libs/window)
add_subdirectory(libs/graphics)
add_subdirectory(libs/levk_imgui)

# Engine
set(LEVK_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/src/generated/build_version.hpp")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/build_version.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/generated/build_version.hpp")
file(GLOB_RECURSE LEVK_INCLUDES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.?pp")
file(GLOB_RECURSE LEVK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${LEVK_INCLUDES} ${LEVK_SOURCES})
source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${LEVK_GENERATED})
add_library(${TARGET_NAME} ${LEVK_LIBRARY_TYPE} ${LEVK_INCLUDES} ${LEVK_SOURCES} ${LEVK_GENERATED})
add_library(levk::engine ALIAS ${TARGET_NAME})
target_compile_definitions(${TARGET_NAME} PUBLIC $<$<BOOL:${LEVK_EDITOR}>:LEVK_EDITOR>)
target_include_directories(${TARGET_NAME} SYSTEM
	INTERFACE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/src/generated"
)
target_include_directories(${TARGET_NAME}
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/src/generated"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(${TARGET_NAME}
	PUBLIC
		levk::imgui
		levk::graphics
		levk::window
		levk::core
		djson::djson
		clap::clap
		$<$<STREQUAL:${PLATFORM},Android>:levk::android-glue>
		levk::interface
	PRIVATE
		stb-image
		tinyobjloader
)
if(LEVK_USE_PCH AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
	target_precompile_headers(${TARGET_NAME} 
		PUBLIC
			<deque>
			<string>
			<unordered_map>
			<vector>
		PRIVATE
			<vulkan/vulkan.hpp>
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
if(LEVK_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

set(CCJSON "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
if(LEVK_COPY_CCJSON AND EXISTS "${CCJSON}")
	message(STATUS "Copying ${CCJSON} to project root")
	file(COPY "${CCJSON}" DESTINATION "${PROJECT_SOURCE_DIR}")
endif()
