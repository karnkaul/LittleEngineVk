#pragma once
#include <engine/systems/system.hpp>

namespace le {
class SceneCleanSystem : public System {
	void tick(dens::registry const& registry, Time_s dt) override;

  public:
	static constexpr Order order_v = -100;
};
} // namespace le
