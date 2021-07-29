cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)	# don't let OPTION erase above non-cache variables
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
endif()

# includes
include(CheckPIESupported)
check_pie_supported()
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

# options
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
option(LEVK_BUILD_TESTS "Build Tests" ${is_root_project})
option(LEVK_BUILD_DEMO "Build demo" ${is_root_project})

if(LINUX_CLANG OR WINDOWS_CLANG)
	option(LEVK_ASAN "Enable asan" OFF)
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
elseif(LEVK_ASAN)
	set(build_config "${CMAKE_BUILD_TYPE} ASAN")
endif()
message(STATUS "[${CMAKE_SYSTEM_PROCESSOR}] [${PLATFORM}] [${CMAKE_GENERATOR}] [${CMAKE_CXX_COMPILER_ID}] [${build_config}]")
if(CI_BUILD)
	message(STATUS "== CI Build ==")
endif()

# vulkan
add_subdirectory(ext/vulkan_hpp)

# external
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

# engine
set(LEVK_LIBRARY_TYPE STATIC CACHE STRING "levk library type (STATIC/SHARED)")

if(PLATFORM STREQUAL Win64 AND NOT LEVK_LIBRARY_TYPE STREQUAL STATIC)
	message(FATAL_ERROR "`LEVK_LIBRARY_TYPE` must be `STATIC` on Windows; DLLs are not supported!")
endif()
if(NOT LEVK_LIBRARY_TYPE STREQUAL STATIC AND LEVK_USE_PCH)
	message(WARNING "Cannot use PCH with a shared library!")
	set(LEVK_USE_PCH OFF CACHE BOOL "" FORCE)
endif()

# ktl
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
	ktl
	GIT_REPOSITORY https://github.com/karnkaul/ktl
	GIT_TAG origin/main
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/ktl"
)
FetchContent_MakeAvailable(ktl)

# libraries
if(is_root_project AND NOT CI_BUILD)
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
		$<$<STREQUAL:${PLATFORM},Android>:levk::android-glue>
		levk::interface
	PRIVATE
		stb-image
		tinyobjloader
)
if(LEVK_USE_PCH AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
	target_precompile_headers(${target_name} 
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

# demo
if(LEVK_BUILD_DEMO)
	add_subdirectory(demo)
endif()

# tests
if(LEVK_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()
