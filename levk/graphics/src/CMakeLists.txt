target_sources(${PROJECT_NAME} PRIVATE
  buffer.cpp
  command_buffer.cpp
  geometry.cpp
  image.cpp
  memory.cpp
  mesh_primitive.cpp
  mesh.cpp
  skybox.cpp
  texture.cpp
  texture_atlas.cpp

  font/atlas.cpp
  font/face.cpp
  font/font.cpp

  ft/ft.cpp
  ft/ft.hpp

  gltf/bytestream.cpp
  gltf/gltf.cpp
)

add_subdirectory(device)
add_subdirectory(render)
add_subdirectory(utils)
