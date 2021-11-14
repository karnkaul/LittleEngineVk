#pragma once
#include <engine/systems/system.hpp>

namespace le {
struct SpringArm;

template <>
class System<SpringArm> : public SystemBase {
  public:
	void tick(dens::registry const& registry, Time_s dt) override;
};
} // namespace le
