#include <engine/game/spring_arm.hpp>

namespace le {
glm::vec3 const& SpringArm_OLD::tick(Time_s dt, std::optional<glm::vec3> target) noexcept {
	static constexpr u32 max = 64;
	if (pTarget || target) {
		if (!target) {
			target = pTarget->position();
		}
		if (dt.count() > fixed.count() * f32(max)) {
			position = *target + offset;
		} else {
			data.ft += dt;
			for (u32 iter = 0; data.ft.count() > 0.0f; ++iter) {
				if (iter == max) {
					position = *target + offset;
					break;
				}
				Time_s const diff = data.ft > fixed ? fixed : data.ft;
				data.ft -= diff;
				auto const disp = *target + offset - position;
				data.velocity = (1.0f - b) * data.velocity + k * disp;
				position += (diff.count() * data.velocity);
			}
		}
	}
	return position;
}
} // namespace le
