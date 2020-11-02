#include <engine/game/spring_arm.hpp>

namespace le {
glm::vec3 const& SpringArm::tick(Time dt, std::optional<glm::vec3> target) noexcept {
	static constexpr u32 max = 64;
	if (pTarget || target) {
		if (!target) {
			target = pTarget->position();
		}
		if (dt > fixed.scaled(max)) {
			position = *target + offset;
		} else {
			data.ft += dt;
			for (u32 iter = 0; data.ft > Time(); ++iter) {
				if (iter == max) {
					position = *target + offset;
					break;
				}
				Time const diff = data.ft > fixed ? fixed : data.ft;
				data.ft -= diff;
				auto const disp = *target + offset - position;
				data.velocity = (1.0f - b) * data.velocity + k * disp;
				position += (diff.to_s() * data.velocity);
			}
		}
	}
	return position;
}
} // namespace le
