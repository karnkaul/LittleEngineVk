#pragma once
#include <engine/ecs/systems/component_system.hpp>

namespace le {
class SceneCleanSystem : public ComponentSystem {
	void update(dens::registry const& registry) override;

  public:
	static constexpr Order order_v = -100;
};
} // namespace le
