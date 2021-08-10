project(levk-demo)

# library
add_subdirectory(lib)

# executable
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX src FILES main.cpp)
add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} levk::demo::lib levk::interface::exe)
if(WINDOWS_MSBUILD)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
