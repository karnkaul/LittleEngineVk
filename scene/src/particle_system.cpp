#include <le/scene/particle_system.hpp>
#include <le/scene/scene.hpp>

namespace le {
auto ParticleSystem::respawn_all() -> void {
	for (auto& emitter : emitters) { emitter.respawn_all(get_scene().main_camera.view()); }
}

auto ParticleSystem::tick(Duration dt) -> void {
	for (auto& emitter : emitters) { emitter.update(get_scene().main_camera.view(), dt); }
}

auto ParticleSystem::render_to(std::vector<graphics::RenderObject>& out) const -> void {
	out.reserve(out.size() + emitters.size());
	auto const parent = get_scene().get_node_tree().global_transform(get_entity().get_node());
	for (auto const& emitter : emitters) {
		auto object = emitter.render_object();
		if (object.instances.empty()) { continue; }
		object.parent = parent * object.parent;
		out.push_back(object);
	}
}
} // namespace le
