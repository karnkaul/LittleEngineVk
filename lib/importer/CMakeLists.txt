project(levk-importer VERSION ${build_version})

add_library(${PROJECT_NAME})
add_library(levk::importer ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  levk::lib
  gltf2cpp::gltf2cpp

  PRIVATE
  levk::compile-options
)

target_precompile_headers(${PROJECT_NAME} REUSE_FROM levk-lib)

target_sources(${PROJECT_NAME} PRIVATE
  include/levk/importer.hpp
  src/importer.cpp
)
