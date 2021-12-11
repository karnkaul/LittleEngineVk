target_sources(${PROJECT_NAME} PRIVATE
  bitmap_font.cpp
  command_buffer.cpp
  geometry.cpp
  glyph_pen.cpp
  glyph.cpp
  memory.cpp
  mesh_primitive.cpp
  texture.cpp
)

add_subdirectory(context)
add_subdirectory(render)
add_subdirectory(utils)
