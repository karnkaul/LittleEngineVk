add_subdirectory(freetype)

target_sources(${PROJECT_NAME} PRIVATE
  font_atlas.cpp
  font_library.cpp
  font.cpp
  glyph_page.cpp
)
