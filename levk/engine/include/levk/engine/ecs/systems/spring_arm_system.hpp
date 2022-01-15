#pragma once
#include <levk/engine/ecs/systems/component_system.hpp>

namespace le {
class SpringArmSystem : public ComponentSystem {
	void update(dens::registry const& registry) override;
};
} // namespace le
