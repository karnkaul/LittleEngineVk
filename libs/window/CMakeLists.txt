project(levk-window)

# Library
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(levk::window ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(${PROJECT_NAME}
	PUBLIC
		levk::core
	PRIVATE
		levk::interface
		levk::vulkan-hpp
		$<$<STREQUAL:${PLATFORM},Android>:levk::android-glue>
		$<$<BOOL:${LEVK_USE_GLFW}>:glfw>
)

target_compile_options(${PROJECT_NAME} PRIVATE $<$<STREQUAL:${PLATFORM},Linux>:-fPIC>)
target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<BOOL:${LEVK_USE_GLFW}>:LEVK_USE_GLFW LEVK_DESKTOP>)
# PCH
if(LEVK_USE_PCH AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.17")
	target_precompile_headers(${PROJECT_NAME} REUSE_FROM levk-core)
endif()
