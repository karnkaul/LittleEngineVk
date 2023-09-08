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

function(add_le_test NAME SOURCES)
  add_executable(${PROJECT_NAME}-${NAME})
  target_sources(${PROJECT_NAME}-${NAME} PRIVATE ${SOURCES})
  target_link_libraries(${PROJECT_NAME}-${NAME} PRIVATE ${PROJECT_NAME})
  add_test(NAME ${PROJECT_NAME}-${NAME} COMMAND ${PROJECT_NAME}-${NAME})
endfunction()

add_le_test(result test_result.cpp)
