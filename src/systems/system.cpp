#include <core/utils/expect.hpp>
#include <engine/systems/system.hpp>

namespace le {
struct System::Scope {
	System& system;

	Scope(System& system, dts::scheduler& scheduler) noexcept : system(system) { system.m_scheduler = &scheduler; }
	~Scope() {
		system.m_scheduler->wait_stages(system.m_wait);
		system.m_wait.clear();
		system.m_scheduler = {};
	}
};

System::StageID System::stageTasks(Stage&& stage) {
	if (m_scheduler) {
		auto const ret = m_scheduler->stage(std::move(stage));
		m_wait.push_back(ret);
		return ret;
	}
	return {};
}

void System::update(dts::scheduler& scheduler, dens::registry const& registry, Time_s dt) {
	Scope scope(*this, scheduler);
	tick(registry, dt);
}
} // namespace le
