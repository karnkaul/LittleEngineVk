add_subdirectory(ui)
add_subdirectory(imcpp)

target_sources(${PROJECT_NAME} PRIVATE
  collision.cpp
  component.cpp
  entity.cpp
  freecam_controller.cpp
  mesh_animator.cpp
  mesh_renderer.cpp
  particle_system.cpp
  runtime.cpp
  scene_manager.cpp
  scene_renderer.cpp
  scene.cpp
  shape_renderer.cpp
)
