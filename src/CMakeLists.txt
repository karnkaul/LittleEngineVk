project(levk-engine)

set(LEVK_LIBRARY_TYPE STATIC CACHE STRING "levk library type (STATIC/SHARED)")

if(PLATFORM STREQUAL "Win64" AND NOT LEVK_LIBRARY_TYPE STREQUAL STATIC)
	message(FATAL_ERROR "`LEVK_LIBRARY_TYPE` must be `STATIC` on Windows; DLLs are not supported!")
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
set(GENERATED_SOURCES
	"${CMAKE_CURRENT_BINARY_DIR}/generated/version/build_version.hpp"
)
configure_file_src_to_bin(version/build_version.hpp.in generated/version/build_version.hpp)
glob_sources(LEVK_INCLUDES "../include/*.?pp")
glob_sources(LEVK_SOURCES "../src/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/.." FILES ${LEVK_INCLUDES} ${LEVK_SOURCES})
source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${GENERATED_SOURCES})
add_library(${PROJECT_NAME} ${LEVK_LIBRARY_TYPE} ${LEVK_INCLUDES} ${LEVK_SOURCES} ${GENERATED_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE 
	INTERFACE SYSTEM
		"${CMAKE_CURRENT_SOURCE_DIR}/../include"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/../include"
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_CURRENT_BINARY_DIR}/generated"
		"${Vulkan_INCLUDE_DIRS}"
)
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		levk-core
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
		<deque>
		<filesystem>
		<string>
		<unordered_map>
		<vector>
		<vulkan/vulkan.hpp>
		<vk_mem_alloc.h>
		<stb/stb_image.h>
		<tinyobjloader/tiny_obj_loader.h>
	)
	target_precompile_headers(${PROJECT_NAME} PRIVATE ${PRECOMPILE})
endif()

