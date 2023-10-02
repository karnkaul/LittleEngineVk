project(le-test)

add_library(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
  test/test.cpp
  test/test.hpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC
  le::little-engine
  le::le-compile-options
)

target_include_directories(${PROJECT_NAME} PUBLIC
  .
)

add_subdirectory(tests)
