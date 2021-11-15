#include <core/transform.hpp>
#include <engine/components/spring_arm.hpp>
#include <engine/systems/spring_arm_system.hpp>

namespace le {
void SpringArmSystem::tick(dens::registry const& registry, Time_s dt) {
	static constexpr u32 max = 64;
	for (auto [e, c] : registry.view<SpringArm, Transform>()) {
		auto& [spring, transform] = c;
		if (auto target = registry.find<Transform>(spring.target)) {
			if (dt.count() > spring.fixed.count() * f32(max)) {
				transform.position(target->position() + spring.offset);
			} else {
				spring.data.ft += dt;
				for (u32 iter = 0; spring.data.ft.count() > 0.0f; ++iter) {
					if (iter == max) {
						transform.position(target->position() + spring.offset);
						break;
					}
					Time_s const diff = spring.data.ft > spring.fixed ? spring.fixed : spring.data.ft;
					spring.data.ft -= diff;
					auto const disp = target->position() + spring.offset - transform.position();
					spring.data.velocity = (1.0f - spring.b) * spring.data.velocity + spring.k * disp;
					transform.position(transform.position() + (diff.count() * spring.data.velocity));
				}
			}
		}
	}
}
} // namespace le