project(levk-graphics)

# Library
add_library(${PROJECT_NAME})
add_library(levk::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<BOOL:${LEVK_USE_IMGUI}>:LEVK_USE_IMGUI>)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    src
)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    dyvk::dyvk
    spirv-cross-glsl
    vma
    levk::levk-core
  PRIVATE
    levk::levk-compile-options
    stb-image
    tinyobjloader
    freetype
)

include(graphics_includes.cmake)
add_subdirectory(src)
target_source_group(TARGET ${PROJECT_NAME})

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

if(LEVK_INSTALL)
  install(TARGETS levk-graphics EXPORT levk-graphics-targets)
  install(EXPORT levk-graphics-targets
    FILE levk-graphics-targets.cmake
    NAMESPACE levk::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/levk
  )
  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
  )
endif()
