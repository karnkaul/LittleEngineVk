project(levk-graphics)

# Library
add_library(${PROJECT_NAME})
add_library(levk::graphics ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
  PUBLIC
    include
  PRIVATE
    src
)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    vk-hpp::vk-hpp
    spirv-cross-glsl
    vma
    levk::core
  PRIVATE
    levk::compile-options
    stb-image
    tinyobjloader
)

target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<BOOL:${LEVK_USE_IMGUI}>:LEVK_USE_IMGUI>)

add_subdirectory(include/graphics)
add_subdirectory(src)
target_source_group(${PROJECT_NAME})

# PCH
if(LEVK_USE_PCH)
  target_precompile_headers(${PROJECT_NAME}
    PUBLIC
      <string>
      <unordered_map>
      <vector>
      <vulkan/vulkan.hpp>
    PRIVATE
      <stb/stb_image.h>
      <tinyobjloader/tiny_obj_loader.h>
  )
endif()
