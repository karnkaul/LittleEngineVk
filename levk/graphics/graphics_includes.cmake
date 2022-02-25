target_sources(${PROJECT_NAME} PRIVATE
  include/levk/graphics/basis.hpp
  include/levk/graphics/buffer.hpp
  include/levk/graphics/command_buffer.hpp
  include/levk/graphics/common.hpp
  include/levk/graphics/draw_view.hpp
  include/levk/graphics/geometry.hpp
  include/levk/graphics/image_ref.hpp
  include/levk/graphics/image.hpp
  include/levk/graphics/material_data.hpp
  include/levk/graphics/memory.hpp
  include/levk/graphics/mesh_primitive.hpp
  include/levk/graphics/primitive_view.hpp
  include/levk/graphics/mesh.hpp
  include/levk/graphics/qtype.hpp
  include/levk/graphics/rgba.hpp
  include/levk/graphics/screen_rect.hpp
  include/levk/graphics/skybox.hpp
  include/levk/graphics/texture.hpp
  include/levk/graphics/texture_atlas.hpp

  include/levk/graphics/device/defer_queue.hpp
  include/levk/graphics/device/device.hpp
  include/levk/graphics/device/physical_device.hpp
  include/levk/graphics/device/queue.hpp
  include/levk/graphics/device/transfer.hpp
  include/levk/graphics/device/vram.hpp

  include/levk/graphics/font/atlas.hpp
  include/levk/graphics/font/glyph.hpp
  include/levk/graphics/font/face.hpp
  include/levk/graphics/font/font.hpp

  include/levk/graphics/gltf/bytestream.hpp
  include/levk/graphics/gltf/data_builder.hpp
  include/levk/graphics/gltf/gltf.hpp

  include/levk/graphics/render/buffering.hpp
  include/levk/graphics/render/camera.hpp
  include/levk/graphics/render/context.hpp
  include/levk/graphics/render/descriptor_set.hpp
  include/levk/graphics/render/pipeline_factory.hpp
  include/levk/graphics/render/pipeline_flags.hpp
  include/levk/graphics/render/pipeline_spec.hpp
  include/levk/graphics/render/pipeline.hpp
  include/levk/graphics/render/render_pass.hpp
  include/levk/graphics/render/renderer.hpp
  include/levk/graphics/render/shader_buffer.hpp
  include/levk/graphics/render/surface.hpp
  include/levk/graphics/render/framebuffer.hpp
  include/levk/graphics/render/vertex_input.hpp
  include/levk/graphics/render/vsync.hpp

  include/levk/graphics/utils/command_pool.hpp
  include/levk/graphics/utils/defer.hpp
  include/levk/graphics/utils/instant_command.hpp
  include/levk/graphics/utils/layout_state.hpp
  include/levk/graphics/utils/trotator.hpp
  include/levk/graphics/utils/utils.hpp
  include/levk/graphics/utils/quad_uv.hpp
)
