add_subdirectory(cache)
add_subdirectory(animation)

target_sources(${PROJECT_NAME} PRIVATE
  allocator.cpp
  camera.cpp
  command_buffer.cpp
  dear_imgui.cpp
  descriptor_updater.cpp
  defer.cpp
  device.cpp
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
  render_pass.cpp
  shader_layout.cpp
  swapchain.cpp
  texture.cpp
)
