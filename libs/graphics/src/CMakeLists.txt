target_sources(${PROJECT_NAME} PRIVATE
  bitmap_font.cpp
  buffer.cpp
  command_buffer.cpp
  geometry.cpp
  glyph_pen.cpp
  bitmap_glyph.cpp
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
