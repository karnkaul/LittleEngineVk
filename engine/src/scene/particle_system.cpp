#include <spaced/scene/particle_system.hpp>
#include <spaced/scene/scene.hpp>

namespace spaced {
auto ParticleSystem::tick(Duration dt) -> void {
	for (auto& emitter : emitters) { emitter.update(get_scene().main_camera.view(), dt); }
}

auto ParticleSystem::render_to(std::vector<graphics::RenderObject>& out) const -> void {
	out.reserve(out.size() + emitters.size());
	auto const parent = get_scene().get_node_tree().global_transform(get_entity().get_node());
	for (auto const& emitter : emitters) {
		auto object = emitter.render_object();
		object.parent = parent * object.parent;
		out.push_back(object);
	}
}
} // namespace spaced
