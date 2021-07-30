project(levk-graphics)

# Library
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(levk::graphics ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_include_directories(${PROJECT_NAME} SYSTEM 
	PUBLIC
		"${SPIRV-Cross_SOURCE}"
)
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		levk::core
		levk::vulkan-hpp
		spirv-cross-glsl
		vma
	PRIVATE
		levk::interface
		stb-image
		tinyobjloader
)

target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<BOOL:${LEVK_USE_IMGUI}>:LEVK_USE_IMGUI>)

# PCH
if(LEVK_USE_PCH AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
	target_precompile_headers(${PROJECT_NAME} 
		PUBLIC
			<deque>
			<filesystem>
			<string>
			<unordered_map>
			<vector>
			<vulkan/vulkan.hpp>
		PRIVATE
			<stb/stb_image.h>
			<tinyobjloader/tiny_obj_loader.h>
	)
endif()
