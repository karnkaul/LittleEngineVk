project(le-importer-lib)

add_library(${PROJECT_NAME})
add_library(le::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC
  le::little-engine
  gltf2cpp::gltf2cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  le::le-compile-options
)

target_include_directories(${PROJECT_NAME} PUBLIC
  include
)

target_sources(${PROJECT_NAME} PRIVATE
  include/le/importer/importer.hpp
  src/importer.cpp
)
