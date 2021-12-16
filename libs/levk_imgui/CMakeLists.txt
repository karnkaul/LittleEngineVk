project(levk-imgui)

# Library
add_library(${PROJECT_NAME})
add_library(levk::imgui ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
  PUBLIC
    include
  PRIVATE
    src
)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    $<$<BOOL:${LEVK_USE_IMGUI}>:dear-imgui::dear-imgui>
    levk::core
    levk::graphics
    levk::window
  PRIVATE
    levk::interface
    $<$<BOOL:${LEVK_USE_GLFW}>:glfw>
)

target_sources(${PROJECT_NAME} PRIVATE
  include/levk_imgui/levk_imgui.hpp
  src/levk_imgui.cpp
)
target_source_group(${PROJECT_NAME})

# PCH
if(LEVK_USE_PCH)
  target_precompile_headers(${PROJECT_NAME} REUSE_FROM levk-graphics)
endif()
