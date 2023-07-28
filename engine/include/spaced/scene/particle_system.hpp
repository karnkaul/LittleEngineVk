#pragma once
#include <spaced/graphics/particle.hpp>
#include <spaced/scene/render_component.hpp>

namespace spaced {
class ParticleSystem : public RenderComponent {
  public:
	using Particle = graphics::Particle;

	std::vector<Particle::Emitter> emitters{};

	auto tick(Duration dt) -> void override;
	auto render_to(std::vector<graphics::RenderObject>& out) const -> void override;
};
} // namespace spaced
