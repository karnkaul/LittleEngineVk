add_subdirectory(ui)

target_sources(${PROJECT_NAME} PRIVATE
  component.cpp
  entity.cpp
  freecam_controller.cpp
  mesh_animator.cpp
  mesh_renderer.cpp
  particle_system.cpp
  scene_manager.cpp
  scene_renderer.cpp
  scene.cpp
)
