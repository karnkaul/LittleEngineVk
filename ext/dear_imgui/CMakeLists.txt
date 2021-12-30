project(dear-imgui)

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
  src/backends/imgui_impl_glfw.cpp
  src/backends/imgui_impl_glfw.h
  src/backends/imgui_impl_vulkan.cpp
  src/backends/imgui_impl_vulkan.h
  src/imconfig.h
  src/imgui.h
  src/imconfig.h
  src/imgui_demo.cpp
  src/imgui_draw.cpp
  src/imgui_internal.h
  src/imgui_tables.cpp
  src/imgui_widgets.cpp
  src/imgui.cpp
  src/imstb_rectpack.h
  src/imstb_textedit.h
  src/imstb_truetype.h
)
target_source_group(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC src)
target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    glfw
    dyvk::dyvk
)
