project(le-example)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE
  le::little-engine # core engine
  le::le-scene # scene wrappers (links to engine so the above isn't required here)
  le::le-compile-options # compiler flags
  le::le-importer-lib # GLTF mesh Importer
)

target_sources(${PROJECT_NAME} PRIVATE
  example.cpp
)

# extract data: contains BrainStem mesh and Vera font.
message(STATUS "extracting data...")
file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/data.zip"
  DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/data/"
)

# add a target to build shaders into data directory.
add_custom_target(le-example-compile-shaders ALL glsl2spirv "${CMAKE_CURRENT_SOURCE_DIR}/../shaders" "${CMAKE_CURRENT_BINARY_DIR}/data/shaders"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  USES_TERMINAL
  COMMENT "Compiling shaders for le-example"
)
