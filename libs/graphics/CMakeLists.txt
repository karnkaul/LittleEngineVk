project(levk-graphics)

# Library
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME}
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_include_directories(${PROJECT_NAME} SYSTEM
	PUBLIC
		"${SPIRV-Cross_SOURCE}"
		"${Vulkan_INCLUDE_DIRS}"
)
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		levk-core
		spirv-cross-glsl
		vma
		${Vulkan_LIBRARIES}

	PRIVATE
		levk-interface
		stb-image
		tinyobjloader
		$<$<STREQUAL:${PLATFORM},Linux>:X11>
)

# PCH
if(LEVK_USE_PCH AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
	target_precompile_headers(${PROJECT_NAME} 
		PUBLIC
			<deque>
			<filesystem>
			<string>
			<unordered_map>
			<vector>
			<vk_mem_alloc.h>
		PRIVATE
			<stb/stb_image.h>
			<tinyobjloader/tiny_obj_loader.h>
	)
endif()
