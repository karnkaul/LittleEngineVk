project(levk-window)

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
	PRIVATE
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
)

# PCH
if(LEVK_USE_PCH AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
	target_precompile_headers(${PROJECT_NAME} REUSE_FROM levk-core)
endif()
