add_subdirectory(core)
add_subdirectory(input)
add_subdirectory(node)
add_subdirectory(vfs)
add_subdirectory(resources)
add_subdirectory(graphics)
add_subdirectory(scene)
add_subdirectory(imcpp)

target_sources(${PROJECT_NAME} PRIVATE
  build_version.cpp
  engine.cpp
  runtime.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE .)
