#pragma once
#include <engine/systems/system.hpp>

namespace le {
class SpringArmSystem : public System {
	void tick(dens::registry const& registry, Time_s dt) override;
};
} // namespace le
