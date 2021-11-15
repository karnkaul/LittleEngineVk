#pragma once
#include <engine/systems/component_system.hpp>

namespace le {
class PhysicsSystem : public ComponentSystem {
	void update(dens::registry const& registry) override;
};
} // namespace le
