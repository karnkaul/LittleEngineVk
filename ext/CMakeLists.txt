find_package(Threads)

file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src.zip" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(src/dyvk)

add_subdirectory(src/vma)
add_library(vma::vma ALIAS vma)

set(GLFW_VULKAN_STATIC OFF)
set(GLFW_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
add_subdirectory(src/glfw)
add_library(glfw::glfw ALIAS glfw)

add_subdirectory(src/glm)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(glm INTERFACE -Wno-volatile)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  target_compile_options(glm INTERFACE -Wno-deprecated-volatile)
endif()

add_subdirectory(src/dear_imgui)
add_library(dear_imgui::dear_imgui ALIAS dear_imgui)

add_subdirectory(src/stb)
add_library(stb::image ALIAS stb-image)

add_subdirectory(src/djson)

set(GLTF2CPP_BUILD_TESTS OFF)
add_subdirectory(src/gltf2cpp)

if(SPACED_USE_FREETYPE)
  add_subdirectory(src/freetype)
endif()

add_subdirectory(src/portable-file-dialogs)

set(CAPO_BUILD_EXAMPLES OFF)
add_subdirectory(src/capo-lite)

set(CLAP_BUILD_EXAMPLE OFF)
set(CLAP_BUILD_TEST OFF)
add_subdirectory(src/clap)

add_library(spaced-ext INTERFACE)
add_library(spaced::spaced-ext ALIAS spaced-ext)

target_link_libraries(spaced-ext INTERFACE
  dyvk::dyvk
  vma::vma
  glfw::glfw
  glm::glm
  dear_imgui::dear_imgui
  stb::image
  djson::djson
  capo::lite

  $<$<BOOL:${SPACED_USE_FREETYPE}>:freetype::freetype>
  Threads::Threads
)

target_compile_definitions(glm INTERFACE
  GLM_FORCE_XYZW_ONLY
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_SILENT_WARNINGS
  GLM_ENABLE_EXPERIMENTAL
  GLM_EXT_INCLUDED
)
