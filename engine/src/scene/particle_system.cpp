#include <spaced/scene/particle_system.hpp>
#include <spaced/scene/scene.hpp>

namespace spaced {
auto ParticleSystem::tick(Duration dt) -> void {
	for (auto& emitter : emitters) { emitter.update(get_scene().camera.view(), dt); }
}

auto ParticleSystem::render_to(std::vector<graphics::RenderObject>& out) const -> void {
	out.reserve(out.size() + emitters.size());
	for (auto const& emitter : emitters) { out.push_back(emitter.render_object()); }
}
} // namespace spaced
