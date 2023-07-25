#include <spaced/engine/graphics/descriptor_updater.hpp>
#include <spaced/engine/graphics/render_camera.hpp>

namespace spaced::graphics {
auto RenderCamera::bind_set(glm::vec2 const projection, vk::CommandBuffer const cmd) const -> void {
	auto const view = Std140View{
		.mat_vp = camera.projection(projection) * camera.view(),
		.vpos_exposure = {camera.transform.position(), camera.exposure},
	};

	auto dir_lights = std::vector<Std430DirLight>{};
	dir_lights.reserve(lights.directional.size());
	for (auto const& in : lights.directional) {
		dir_lights.push_back(Std430DirLight{
			.direction = glm::vec4{in.direction.value(), 0.0f},
			.diffuse = in.diffuse.to_vec4(),
			.ambient = in.diffuse.Rgba::to_vec4(in.ambient),
		});
	}

	auto const& layout = PipelineCache::self().shader_layout().camera;
	DescriptorUpdater{layout.set}
		.write_storage(layout.directional_lights, dir_lights.data(), std::span{dir_lights}.size_bytes())
		.write_uniform(layout.view, &view, sizeof(view))
		.bind_set(cmd);
}
} // namespace spaced::graphics
