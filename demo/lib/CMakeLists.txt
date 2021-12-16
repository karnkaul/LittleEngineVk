project(levk-demo-lib)

# Executable
add_library(${PROJECT_NAME} STATIC)
add_library(levk::demo::lib ALIAS ${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE demo.hpp demo.cpp)
target_source_group(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME} PUBLIC levk::engine levk::compile-options)
if(LEVK_USE_PCH)
  target_precompile_headers(${PROJECT_NAME} REUSE_FROM levk-engine)
endif()
