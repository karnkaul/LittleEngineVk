project(spaced-importer VERSION 0.1.0)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE
  spaced::spaced-engine
  spaced::spaced-compile-options

  clap::clap
  gltf2cpp::gltf2cpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  SPACED_IMPORTER_VERSION="${PROJECT_VERSION}"
)

target_sources(${PROJECT_NAME} PRIVATE
  importer.cpp
)
