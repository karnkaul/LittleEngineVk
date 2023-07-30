add_subdirectory(cache)
add_subdirectory(animation)
add_subdirectory(font)

target_sources(${PROJECT_NAME} PRIVATE
  allocator.cpp
  camera.cpp
  command_buffer.cpp
  dear_imgui.cpp
  descriptor_updater.cpp
  defer.cpp
  device.cpp
  dynamic_atlas.cpp
  fallback.cpp
  geometry.cpp
  image_file.cpp
  image_barrier.cpp
  material.cpp
  particle.cpp
  primitive.cpp
  rgba.cpp
  renderer.cpp
  resource.cpp
  shader_layout.cpp
  subpass.cpp
  swapchain.cpp
  texture.cpp
)
