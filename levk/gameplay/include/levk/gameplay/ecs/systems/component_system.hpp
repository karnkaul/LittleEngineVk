#pragma once
#include <dens/registry.hpp>
#include <dens/system.hpp>
#include <dumb_tasks/executor.hpp>
#include <levk/core/time.hpp>
#include <levk/core/utils/vbase.hpp>
#include <levk/engine/engine.hpp>

namespace le {
struct SystemData {
	Engine::Service engine;
	Time_s dt{};
};

class ComponentSystem : public dens::system<SystemData> {
  public:
	using Order = dens::order_t;

  private:
	std::vector<dts::future_t> m_wait;
};
} // namespace le
