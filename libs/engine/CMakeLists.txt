project(levk)

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
	message("== Vulkan ==\n\tHeaders : ${Vulkan_INCLUDE_DIRS}\n\tLibraries : ${Vulkan_LIBRARIES}")
endif()

# Library
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/*.*pp"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "src" FILES ${SOURCES})
set(GENERATED_SOURCES
	"${CMAKE_CURRENT_BINARY_DIR}/generated/version/build_version.hpp"
)
configure_file_src_to_bin(src/version/build_version.hpp.in generated/version/build_version.hpp)
source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${GENERATED_SOURCES})
add_library(${PROJECT_NAME} ${LEVK_LIBRARY_TYPE} ${SOURCES} ${GENERATED_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<BOOL:${LEVK_USE_GLFW}>:LEVK_USE_GLFW>)
add_target_compile_options(${PROJECT_NAME} PRIVATE)
target_include_directories(${PROJECT_NAME} PRIVATE 
	INTERFACE SYSTEM
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${CMAKE_CURRENT_BINARY_DIR}/generated"
		"${Vulkan_INCLUDE_DIRS}"
)
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		core
	PRIVATE
		${Vulkan_LIBRARIES}
		$<$<STREQUAL:${PLATFORM},Linux>:X11>
		$<$<BOOL:${LEVK_USE_GLFW}>:glfw>
		#stb-image
		#tinyobjloader
)
if(WIN64_CLANG AND LEVK_USE_GLFW)
	target_compile_options(glfw PRIVATE -Wno-everything)
endif()
