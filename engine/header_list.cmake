set(prefix include/le)

set(core_headers
  ${prefix}/core/enum_array.hpp
  ${prefix}/core/enumerate.hpp
  ${prefix}/core/fixed_string.hpp
  ${prefix}/core/hash_combine.hpp
  ${prefix}/core/id.hpp
  ${prefix}/core/inclusive_range.hpp
  ${prefix}/core/logger.hpp
  ${prefix}/core/mono_instance.hpp
  ${prefix}/core/named_type.hpp
  ${prefix}/core/not_null.hpp
  ${prefix}/core/nvec3.hpp
  ${prefix}/core/offset_span.hpp
  ${prefix}/core/ptr.hpp
  ${prefix}/core/radians.hpp
  ${prefix}/core/random.hpp
  ${prefix}/core/reversed.hpp
  ${prefix}/core/signal.hpp
  ${prefix}/core/time.hpp
  ${prefix}/core/version.hpp
  ${prefix}/core/visitor.hpp
  ${prefix}/core/wrap.hpp
  ${prefix}/core/zip_ranges.hpp
)

set(node_headers
  ${prefix}/node/node_tree_serializer.hpp
  ${prefix}/node/node_tree.hpp
  ${prefix}/node/node.hpp
)

set(input_headers
  ${prefix}/input/action.hpp
  ${prefix}/input/gamepad.hpp
  ${prefix}/input/key_axis.hpp
  ${prefix}/input/range.hpp
  ${prefix}/input/receiver.hpp
  ${prefix}/input/state.hpp
  ${prefix}/input/trigger.hpp
)

set(vfs_headers
  ${prefix}/vfs/cached_file_reader.hpp
  ${prefix}/vfs/file_reader.hpp
  ${prefix}/vfs/reader.hpp
  ${prefix}/vfs/uri.hpp
  ${prefix}/vfs/vfs.hpp
)

set(resources_headers
  ${prefix}/resources/asset.hpp
  ${prefix}/resources/bin_data.hpp
  ${prefix}/resources/font_asset.hpp
  ${prefix}/resources/material_asset.hpp
  ${prefix}/resources/mesh_asset.hpp
  ${prefix}/resources/primitive_asset.hpp
  ${prefix}/resources/resources.hpp
  ${prefix}/resources/animation_asset.hpp
  ${prefix}/resources/skeleton_asset.hpp
  ${prefix}/resources/texture_asset.hpp
)

set(graphics_headers
  ${prefix}/graphics/cache/descriptor_cache.hpp
  ${prefix}/graphics/cache/pipeline_cache.hpp
  ${prefix}/graphics/cache/sampler_cache.hpp
  ${prefix}/graphics/cache/scratch_buffer_cache.hpp
  ${prefix}/graphics/cache/shader_cache.hpp
  ${prefix}/graphics/cache/vertex_buffer_cache.hpp

  ${prefix}/graphics/animation/interpolator.hpp
  ${prefix}/graphics/animation/animation.hpp

  ${prefix}/graphics/font/codepoint.hpp
  ${prefix}/graphics/font/font_atlas.hpp
  ${prefix}/graphics/font/font_library.hpp
  ${prefix}/graphics/font/font.hpp
  ${prefix}/graphics/font/glyph_page.hpp
  ${prefix}/graphics/font/glyph_slot.hpp
  ${prefix}/graphics/font/text_height.hpp

  ${prefix}/graphics/allocator.hpp
  ${prefix}/graphics/bitmap.hpp
  ${prefix}/graphics/buffering.hpp
  ${prefix}/graphics/camera.hpp
  ${prefix}/graphics/command_buffer.hpp
  ${prefix}/graphics/dear_imgui.hpp
  ${prefix}/graphics/descriptor_updater.hpp
  ${prefix}/graphics/defer.hpp
  ${prefix}/graphics/device.hpp
  ${prefix}/graphics/dynamic_atlas.hpp
  ${prefix}/graphics/fallback.hpp
  ${prefix}/graphics/geometry.hpp
  ${prefix}/graphics/image_file.hpp
  ${prefix}/graphics/image_barrier.hpp
  ${prefix}/graphics/image_view.hpp
  ${prefix}/graphics/lights.hpp
  ${prefix}/graphics/material.hpp
  ${prefix}/graphics/particle.hpp
  ${prefix}/graphics/pipeline_state.hpp
  ${prefix}/graphics/primitive.hpp
  ${prefix}/graphics/rect.hpp
  ${prefix}/graphics/rgba.hpp
  ${prefix}/graphics/render_frame.hpp
  ${prefix}/graphics/render_object.hpp
  ${prefix}/graphics/renderer.hpp
  ${prefix}/graphics/resource.hpp
  ${prefix}/graphics/rgba.hpp
  ${prefix}/graphics/scale_extent.hpp
  ${prefix}/graphics/shader.hpp
  ${prefix}/graphics/shader_layout.hpp
  ${prefix}/graphics/swapchain.hpp
  ${prefix}/graphics/texture_sampler.hpp
  ${prefix}/graphics/texture.hpp
)

set(audio_headers
  ${prefix}/audio/device.hpp
  ${prefix}/audio/music.hpp
  ${prefix}/audio/sound.hpp
  ${prefix}/audio/volume.hpp
)

set(imcpp_headers
  ${prefix}/imcpp/common.hpp
  ${prefix}/imcpp/engine_stats.hpp
  ${prefix}/imcpp/input_text.hpp
  ${prefix}/imcpp/reflector.hpp
)

set(header_list
  ${core_headers}
  ${input_headers}
  ${vfs_headers}
  ${resources_headers}
  ${graphics_headers}
  ${imcpp_headers}
  ${audio_headers}

  ${prefix}/engine.hpp
  ${prefix}/environment.hpp
  ${prefix}/frame_profile.hpp
  ${prefix}/error.hpp
  ${prefix}/stats.hpp
)
