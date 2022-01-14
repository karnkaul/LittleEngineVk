project(levk-demo)

# library
add_subdirectory(lib)

# executable
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cpp)
target_source_group(TARGET ${PROJECT_NAME} PREFIX src)
target_include_directories(${PROJECT_NAME} PRIVATE .)
target_link_libraries(${PROJECT_NAME} levk::demo-lib levk::levk-link-options)

if(WINDOWS_MSBUILD)
  set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()
