add_subdirectory(core)
add_subdirectory(vfs)
add_subdirectory(resources)
add_subdirectory(graphics)

target_sources(${PROJECT_NAME} PRIVATE
  build_version.cpp
  camera.cpp
  engine.cpp
)
