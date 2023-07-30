#include <spaced/core/nvec3.hpp>
#include <spaced/core/random.hpp>
#include <spaced/core/visitor.hpp>
#include <spaced/core/zip_ranges.hpp>
#include <spaced/graphics/device.hpp>
#include <spaced/graphics/particle.hpp>
#include <algorithm>

namespace spaced::graphics {
namespace {
auto random_vec3(Particle::Range<glm::vec3> const& range) -> glm::vec3 {
	auto ret = glm::vec3{};
	ret.x = random_range(range.first.x, range.second.x);
	ret.y = random_range(range.first.y, range.second.y);
	ret.z = random_range(range.first.z, range.second.z);
	return ret;
}

auto translate(Particle& out, Duration dt) -> void { out.position += out.velocity.linear * dt.count(); }
auto rotate(Particle& out, Duration dt) -> void { out.rotation.value += out.velocity.angular.value * dt.count(); }
auto scaleify(Particle& out) -> void { out.scale = glm::mix(out.lerp.scale.first, out.lerp.scale.second, out.alpha); }
auto tintify(Particle& out) -> void { out.tint.channels = glm::mix(out.lerp.tint.first.channels, out.lerp.tint.second.channels, out.alpha); }
} // namespace

auto Particle::Emitter::make_particle() const -> Particle {
	auto ret = Particle{};

	ret.velocity.linear = random_vec3(config.velocity.linear);
	ret.velocity.angular = random_range(config.velocity.angular.first.value, config.velocity.angular.second.value);

	ret.lerp.scale = config.lerp.scale;
	ret.lerp.tint = config.lerp.tint;

	auto const initial_position = [&] {
		auto ret = config.initial.position;
		if (std::abs(ret.first.z - ret.second.z) < 0.01f) {
			// minimize z fighting
			ret.second.z = ret.first.z + 0.1f;
		}
		return ret;
	}();
	ret.position = random_vec3(initial_position);
	ret.rotation = random_range(config.initial.rotation.first.value, config.initial.rotation.second.value);
	ret.scale = config.lerp.scale.first;

	ret.ttl = Duration{random_range(config.ttl.first.count(), config.ttl.second.count())};
	ret.tint = ret.lerp.tint.first;

	return ret;
}

void Particle::Emitter::respawn_all(glm::quat const& view) {
	m_particles.clear();
	m_particles.reserve(config.count);
	auto respawn = config.respawn;
	config.respawn = true;
	update(view, {});
	config.respawn = respawn;
}

void Particle::Emitter::update(glm::quat const& view, Duration dt) {
	std::erase_if(m_particles, [](Particle const& p) { return p.elapsed >= p.ttl; });

	if (config.respawn) {
		m_particles.reserve(config.count);
		while (m_particles.size() < config.count) { m_particles.push_back(make_particle()); }
	}

	m_instances.resize(m_particles.size());

	// invert view to billboard each quad
	auto const view_inverse = glm::inverse(view);
	for (auto [particle, instance] : zip_ranges(m_particles, m_instances)) {
		particle.elapsed += dt;
		particle.alpha = std::clamp(particle.elapsed / particle.ttl, 0.0f, 1.0f);
		if ((modifiers & eTranslate) != 0) { translate(particle, dt); }
		if ((modifiers & eRotate) != 0) { rotate(particle, dt); }
		if ((modifiers & eScale) != 0) { scaleify(particle); }
		if ((modifiers & eTint) != 0) { tintify(particle); }
		auto const orientation = view_inverse * glm::rotate(glm::identity<glm::quat>(), particle.rotation.value, front_v);
		instance.transform.set_orientation(orientation);
		instance.transform.set_position(particle.position);
		instance.transform.set_scale(glm::vec3{particle.scale, 1.0f});
		instance.tint = particle.tint;
	}

	m_primitive->set_geometry(Geometry::from(Quad{.size = config.quad_size}));
}

auto Particle::Emitter::render_object() const -> RenderObject {
	auto ret = RenderObject{
		.material = &material,
		.primitive = m_primitive.get(),
		.parent = transform.matrix(),
		.instances = m_instances,
	};
	return ret;
}
} // namespace spaced::graphics
