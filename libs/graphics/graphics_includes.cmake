target_sources(${PROJECT_NAME} PRIVATE
  include/graphics/basis.hpp
  include/graphics/bitmap_font.hpp
  include/graphics/bitmap.hpp
  include/graphics/buffer.hpp
  include/graphics/command_buffer.hpp
  include/graphics/common.hpp
  include/graphics/draw_view.hpp
  include/graphics/geometry.hpp
  include/graphics/glyph_pen.hpp
  include/graphics/bitmap_glyph.hpp
  include/graphics/image_ref.hpp
  include/graphics/image.hpp
  include/graphics/memory.hpp
  include/graphics/mesh_primitive.hpp
  include/graphics/qtype.hpp
  include/graphics/screen_rect.hpp
  include/graphics/texture.hpp
  include/graphics/texture_atlas.hpp

  include/graphics/context/defer_queue.hpp
  include/graphics/context/device.hpp
  include/graphics/context/physical_device.hpp
  include/graphics/context/queue.hpp
  include/graphics/context/transfer.hpp
  include/graphics/context/vram.hpp

  include/graphics/font/atlas.hpp
  include/graphics/font/glyph.hpp
  include/graphics/font/face.hpp
  include/graphics/font/font.hpp

  include/graphics/render/buffering.hpp
  include/graphics/render/camera.hpp
  include/graphics/render/context.hpp
  include/graphics/render/descriptor_set.hpp
  include/graphics/render/pipeline_factory.hpp
  include/graphics/render/pipeline_flags.hpp
  include/graphics/render/pipeline_spec.hpp
  include/graphics/render/pipeline.hpp
  include/graphics/render/renderer.hpp
  include/graphics/render/rgba.hpp
  include/graphics/render/shader_buffer.hpp
  include/graphics/render/surface.hpp
  include/graphics/render/framebuffer.hpp
  include/graphics/render/vertex_input.hpp
  
  include/graphics/utils/command_pool.hpp
  include/graphics/utils/defer.hpp
  include/graphics/utils/extent2d.hpp
  include/graphics/utils/instant_command.hpp
  include/graphics/utils/layout_state.hpp
  include/graphics/utils/ring_buffer.hpp
  include/graphics/utils/utils.hpp
  include/graphics/utils/quad_uv.hpp
)
