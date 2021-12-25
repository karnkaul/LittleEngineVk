target_sources(${PROJECT_NAME} PRIVATE
  buffer.cpp
  command_buffer.cpp
  geometry.cpp
  image.cpp
  memory.cpp
  mesh_primitive.cpp
  texture.cpp
  texture_atlas.cpp

  font/atlas.cpp
  font/face.cpp
  font/font.cpp

  ft/ft.cpp
  ft/ft.hpp
)

add_subdirectory(context)
add_subdirectory(render)
add_subdirectory(utils)
