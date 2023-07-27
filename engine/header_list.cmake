set(prefix include/spaced/engine)

set(core_headers
  ${prefix}/core/enumerate.hpp
  ${prefix}/core/hash_combine.hpp
  ${prefix}/core/mono_instance.hpp
  ${prefix}/core/logger.hpp
  ${prefix}/core/named_type.hpp
  ${prefix}/core/not_null.hpp
  ${prefix}/core/nvec3.hpp
  ${prefix}/core/ptr.hpp
  ${prefix}/core/signal.hpp
  ${prefix}/core/time.hpp
  ${prefix}/core/visitor.hpp
  ${prefix}/core/zip_ranges.hpp
)

set(input_headers
  ${prefix}/input/state.hpp
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
  ${prefix}/resources/material_asset.hpp
  ${prefix}/resources/mesh_asset.hpp
  ${prefix}/resources/primitive_asset.hpp
  ${prefix}/resources/resources.hpp
  ${prefix}/resources/shader_asset.hpp
  ${prefix}/resources/animation_asset.hpp
  ${prefix}/resources/skeleton_asset.hpp
  ${prefix}/resources/texture_asset.hpp
)

set(graphics_headers
  ${prefix}/graphics/cache/descriptor_cache.hpp
  ${prefix}/graphics/cache/pipeline_cache.hpp
  ${prefix}/graphics/cache/sampler_cache.hpp
  ${prefix}/graphics/cache/scratch_buffer_cache.hpp

  ${prefix}/graphics/animation/interpolator.hpp
  ${prefix}/graphics/animation/animation.hpp

  ${prefix}/graphics/allocator.hpp
  ${prefix}/graphics/bitmap.hpp
  ${prefix}/graphics/buffering.hpp
  ${prefix}/graphics/command_buffer.hpp
  ${prefix}/graphics/dear_imgui.hpp
  ${prefix}/graphics/descriptor_updater.hpp
  ${prefix}/graphics/defer.hpp
  ${prefix}/graphics/device.hpp
  ${prefix}/graphics/fallback.hpp
  ${prefix}/graphics/geometry.hpp
  ${prefix}/graphics/image_file.hpp
  ${prefix}/graphics/image_barrier.hpp
  ${prefix}/graphics/image_view.hpp
  ${prefix}/graphics/lights.hpp
  ${prefix}/graphics/material.hpp
  ${prefix}/graphics/pipeline_state.hpp
  ${prefix}/graphics/primitive.hpp
  ${prefix}/graphics/rect.hpp
  ${prefix}/graphics/rgba.hpp
  ${prefix}/graphics/render_camera.hpp
  ${prefix}/graphics/render_object.hpp
  ${prefix}/graphics/renderer.hpp
  ${prefix}/graphics/resource.hpp
  ${prefix}/graphics/rgba.hpp
  ${prefix}/graphics/render_pass.hpp
  ${prefix}/graphics/shader_layout.hpp
  ${prefix}/graphics/swapchain.hpp
  ${prefix}/graphics/texture_sampler.hpp
  ${prefix}/graphics/texture.hpp
)

set(header_list
  ${core_headers}
  ${input_headers}
  ${vfs_headers}
  ${resources_headers}
  ${graphics_headers}

  ${prefix}/build_version.hpp
  ${prefix}/camera.hpp
  ${prefix}/engine.hpp
  ${prefix}/id.hpp
  ${prefix}/node_tree_serializer.hpp
  ${prefix}/node_tree.hpp
  ${prefix}/node.hpp
  ${prefix}/error.hpp
  ${prefix}/transform.hpp
)
