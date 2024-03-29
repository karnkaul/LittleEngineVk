#pragma once
#include <le/core/inclusive_range.hpp>
#include <le/core/time.hpp>
#include <le/core/transform.hpp>
#include <le/graphics/material.hpp>
#include <le/graphics/primitive.hpp>
#include <le/graphics/render_object.hpp>
#include <le/graphics/rgba.hpp>

namespace le::graphics {
struct Particle {
	struct Config;
	class Emitter;

	enum Flag : std::uint32_t {
		eTranslate = 1 << 0,
		eRotate = 1 << 1,
		eScale = 1 << 2,
		eTint = 1 << 3,
	};

	using Modifiers = std::uint32_t;

	struct {
		glm::vec3 linear{};
		Radians angular{};
	} velocity{};

	struct {
		InclusiveRange<graphics::Rgba> tint{};
		InclusiveRange<glm::vec2> scale{};
	} lerp{};

	Duration ttl{};

	glm::vec3 position{};
	Radians rotation{};
	glm::vec2 scale{};
	graphics::Rgba tint{};
	Duration elapsed{};
	float alpha{};
};

struct Particle::Config {
	struct {
		InclusiveRange<glm::vec3> position{};
		InclusiveRange<Radians> rotation{};
	} initial{};

	struct {
		struct {
			InclusiveRange<Radians> angle{Degrees{-180.0f}, Degrees{180.0f}};
			InclusiveRange<float> speed{-1.0f, 1.0f};
		} linear{};
		InclusiveRange<Radians> angular{Degrees{-90.0f}, Degrees{90.0f}};
	} velocity{};

	struct {
		InclusiveRange<graphics::Rgba> tint{graphics::white_v, graphics::Rgba{.channels = {0xff, 0xff, 0xff, 0x0}}};
		InclusiveRange<glm::vec2> scale{glm::vec3{1.0f}, glm::vec3{0.0f}};
	} lerp{};

	InclusiveRange<Duration> ttl{2s, 10s};
	glm::vec2 quad_size{100.0f};
	std::size_t count{100};
	bool respawn{true};
};

class Particle::Emitter {
  public:
	Config config{};
	graphics::UnlitMaterial material{};
	Modifiers modifiers{eTranslate};
	Transform transform{};

	auto respawn_all(glm::quat const& view) -> void;
	auto update(glm::quat const& view, Duration dt) -> void;
	[[nodiscard]] auto render_object() const -> graphics::RenderObject;

	[[nodiscard]] auto active_particles() const -> std::size_t { return m_particles.size(); }

  private:
	[[nodiscard]] auto make_particle() const -> Particle;

	std::unique_ptr<graphics::DynamicPrimitive> m_primitive{std::make_unique<graphics::DynamicPrimitive>()};
	std::vector<Particle> m_particles{};
	std::vector<graphics::RenderInstance> m_instances{};
};
} // namespace le::graphics
