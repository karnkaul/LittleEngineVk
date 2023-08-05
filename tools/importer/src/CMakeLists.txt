add_subdirectory(lib)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE
  le::le-importer-lib
  le::le-compile-options
  clap::clap
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  LE_IMPORTER_VERSION="${PROJECT_VERSION}"
)

target_sources(${PROJECT_NAME} PRIVATE
  main.cpp
)
