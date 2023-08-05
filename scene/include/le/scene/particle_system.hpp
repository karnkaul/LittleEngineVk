#pragma once
#include <le/graphics/particle.hpp>
#include <le/scene/render_component.hpp>

namespace le {
class ParticleSystem : public RenderComponent {
  public:
	using Particle = graphics::Particle;

	std::vector<Particle::Emitter> emitters{};

	auto tick(Duration dt) -> void override;
	auto render_to(std::vector<graphics::RenderObject>& out) const -> void override;
};
} // namespace le
