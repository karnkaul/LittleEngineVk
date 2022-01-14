#include <core/transform.hpp>
#include <levk/engine/ecs/components/trigger.hpp>
#include <levk/engine/ecs/systems/physics_system.hpp>

namespace le {
using physics::Trigger;

namespace {
constexpr bool intersecting(glm::vec2 l, glm::vec2 r) noexcept {
	return (l.x >= r.x && l.x <= r.y) || (l.y >= r.x && l.y <= r.y) || (r.x >= l.x && r.x <= l.y) || (r.y >= l.x && r.y < l.y);
}

constexpr glm::vec2 lohi(f32 pos, f32 scale, f32 offset) noexcept { return {pos + scale * -0.5f + offset, pos + scale * 0.5f + offset}; }

bool colliding(Trigger const& lt, Trigger const& rt, glm::vec3 const& lp, glm::vec3 const& rp) noexcept {
	glm::vec2 const lx = lohi(lp.x, lt.scale.x, lt.offset.x);
	glm::vec2 const rx = lohi(rp.x, rt.scale.x, rt.offset.x);
	glm::vec2 const ly = lohi(lp.y, lt.scale.y, lt.offset.y);
	glm::vec2 const ry = lohi(rp.y, rt.scale.y, rt.offset.y);
	glm::vec2 const lz = lohi(lp.z, lt.scale.z, lt.offset.z);
	glm::vec2 const rz = lohi(rp.z, rt.scale.z, rt.offset.z);
	return intersecting(lx, rx) && intersecting(ly, ry) && intersecting(lz, rz);
}
} // namespace

void PhysicsSystem::update(dens::registry const& registry) {
	auto view = registry.view<Trigger, Transform>();
	for (std::size_t i = 0; i + 1 < view.size(); ++i) {
		for (std::size_t j = i + 1; j < view.size(); ++j) {
			auto& ltrigger = view[i].get<Trigger>();
			auto& ltransform = view[i].get<Transform>();
			auto& rtrigger = view[j].get<Trigger>();
			auto& rtransform = view[j].get<Transform>();
			bool const ch = (ltrigger.cflags & rtrigger.cflags) != 0;
			if (ch && colliding(ltrigger, rtrigger, ltransform.position(), rtransform.position())) {
				ltrigger.onTrigger(rtrigger);
				rtrigger.onTrigger(ltrigger);
				ltrigger.data.material.Tf = rtrigger.data.material.Tf = colours::red;
			} else {
				ltrigger.data.material.Tf = rtrigger.data.material.Tf = colours::green;
			}
		}
	}
}
} // namespace le
