project(levk-demo)

# Executable
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "src" FILES ${SOURCES})
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE levk-engine levk-interface)
if(WINDOWS_MSBUILD)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
if(LEVK_USE_PCH)
	target_precompile_headers(${PROJECT_NAME} REUSE_FROM levk-engine)
endif()
