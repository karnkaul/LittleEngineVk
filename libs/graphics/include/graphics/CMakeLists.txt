target_sources(${PROJECT_NAME} PRIVATE
  basis.hpp
  bitmap_font.hpp
  command_buffer.hpp
  common.hpp
  draw_view.hpp
  geometry.hpp
  glyph_pen.hpp
  glyph.hpp
  memory.hpp
  mesh_primitive.hpp
  qflags.hpp
  screen_rect.hpp
  texture.hpp
)

add_subdirectory(context)
add_subdirectory(render)
add_subdirectory(utils)
