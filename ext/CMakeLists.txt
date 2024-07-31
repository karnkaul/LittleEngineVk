project(levk-ext)

find_package(Threads)

file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src.zip" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}")
add_subdirectory(src/dyvk)

add_subdirectory(src/vma)
add_library(vma)
add_library(vma::vma ALIAS vma)
target_link_libraries(vma PUBLIC
  GPUOpen::VulkanMemoryAllocator
  dyvk::dyvk
)
target_include_directories(vma SYSTEM PUBLIC src/vma/include)
target_sources(vma PRIVATE src/vma/src/vk_mem_alloc.cpp)
target_compile_definitions(vma PUBLIC
  VMA_STATIC_VULKAN_FUNCTIONS=0
  VMA_DYNAMIC_VULKAN_FUNCTIONS=1
)

set(GLFW_VULKAN_STATIC OFF)
set(GLFW_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
add_subdirectory(src/glfw)
add_library(glfw::glfw ALIAS glfw)

add_subdirectory(src/glm)
target_compile_definitions(glm INTERFACE
  GLM_FORCE_XYZW_ONLY
  GLM_FORCE_RADIANS
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_SILENT_WARNINGS
  GLM_ENABLE_EXPERIMENTAL
  GLM_EXT_INCLUDED
)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(glm INTERFACE -Wno-volatile)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  target_compile_options(glm INTERFACE -Wno-deprecated-volatile)
endif()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(SPIRV_CROSS_CLI OFF)
set(SPIRV_CROSS_ENABLE_CPP OFF)
set(SPIRV_CROSS_ENABLE_C_API OFF)
set(SPIRV_CROSS_ENABLE_GLSL ON)
set(SPIRV_CROSS_ENABLE_HLSL OFF)
set(SPIRV_CROSS_ENABLE_MSL OFF)
set(SPIRV_CROSS_ENABLE_REFLECT OFF)
set(SPIRV_CROSS_ENABLE_TESTS OFF)
set(SPIRV_CROSS_ENABLE_UTIL OFF)
set(SPIRV_CROSS_SHARED OFF)
set(SPIRV_CROSS_STATIC ON)
set(SPIRV_CROSS_SKIP_INSTALL ON)
add_subdirectory(src/spirv_cross)
target_include_directories(spirv-cross-glsl SYSTEM PUBLIC src/spirv_cross)

add_subdirectory(src/dear_imgui)

add_subdirectory(src/stb)
add_library(stb::image ALIAS stb-image)

add_subdirectory(src/djson)

set(GLTF2CPP_DYNARRAY_DEBUG_VIEW ON)
set(GLTF2CPP_BUILD_TESTS ON)
add_subdirectory(src/gltf2cpp)

if(LEVK_USE_FREETYPE)
  add_subdirectory(src/freetype)
endif()

add_subdirectory(src/portable-file-dialogs)

set(CAPO_BUILD_EXAMPLES OFF)
add_subdirectory(src/capo-lite)

add_library(levk-ext INTERFACE)
add_library(levk::ext ALIAS levk-ext)

target_link_libraries(levk-ext INTERFACE
  vma::vma
  glfw::glfw
  glm::glm
  spirv-cross-glsl
  dear_imgui::dear_imgui
  stb::image
  djson::djson
  capo::lite

  $<$<BOOL:${LEVK_USE_FREETYPE}>:freetype::freetype>
  Threads::Threads
)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  target_compile_options(vma PRIVATE -w)
  target_compile_options(spirv-cross-glsl PRIVATE -w)

  if(CAPO_USE_OPENAL)
    target_compile_options(OpenAL PRIVATE -w)
  endif()
endif()
