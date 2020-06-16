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
glob_sources(SOURCES "*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
set(GENERATED_SOURCES
	"${CMAKE_CURRENT_BINARY_DIR}/generated/version/build_version.hpp"
)
configure_file_src_to_bin(version/build_version.hpp.in generated/version/build_version.hpp)
source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${GENERATED_SOURCES})
add_library(${PROJECT_NAME} ${LEVK_LIBRARY_TYPE} ${SOURCES} ${GENERATED_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<BOOL:${LEVK_USE_GLFW}>:LEVK_USE_GLFW> $<$<BOOL:${LEVK_USE_IMGUI}>:LEVK_USE_IMGUI> $<$<BOOL:${LEVK_EDITOR}>:LEVK_EDITOR>)
add_target_compile_definitions(${PROJECT_NAME} LEVK PUBLIC)
add_target_compile_options(${PROJECT_NAME} PRIVATE)
if(LINUX_CLANG AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "9.0.0")
	# Ignore Vulkan.hpp warnings
	message(STATUS "clang ${CMAKE_CXX_COMPILER_VERSION} detected; adding -Wno-deprecated-copy for vulkan.hpp")
	target_compile_options(${PROJECT_NAME} PUBLIC -Wno-deprecated-copy)
endif()
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
		$<$<BOOL:${LEVK_USE_IMGUI}>:imgui>
	PRIVATE
		${Vulkan_LIBRARIES}
		$<$<STREQUAL:${PLATFORM},Linux>:X11>
		$<$<BOOL:${LEVK_USE_GLFW}>:glfw>
		vma
		stb-image
		tinyobjloader
)
if(WIN64_CLANG AND LEVK_USE_GLFW)
	target_compile_options(glfw PRIVATE -Wno-everything)
endif()
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

# Docs
if(LEVK_BUILD_DOCS)
	# 
	set(BUILD_DOCS_CMD echo "Running Doxygen, building docs to ${LEVK_DOXYGEN_DOCS_DIR}..." && rm -rf ${LEVK_DOXYGEN_DOCS_DIR} && doxygen Doxyfile)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${BUILD_DOCS_CMD} WORKING_DIRECTORY "${LEVK_DOXYGEN_DIR}")
endif()
