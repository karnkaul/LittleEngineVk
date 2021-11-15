#pragma once
#include <core/time.hpp>
#include <core/utils/vbase.hpp>
#include <dens/registry.hpp>
#include <dumb_tasks/scheduler.hpp>

namespace le {
class System : public utils::VBase {
  public:
	using Order = s64;

	void update(dts::scheduler& scheduler, dens::registry const& registry, Time_s dt);

  protected:
	using Scheduler = dts::scheduler;
	using Stage = Scheduler::stage_t;
	using StageID = Scheduler::stage_id;

	template <typename F>
	StageID stageTask(F&& func, std::vector<StageID> deps = {});
	StageID stageTasks(Stage&& stage);
	template <typename Container>
	void waitStages(Container const& stageIDs) const;

  private:
	virtual void tick(dens::registry const& registry, Time_s dt) = 0;

	std::vector<dts::scheduler::stage_id> m_wait;
	dts::scheduler* m_scheduler{};

	struct Scope;
	friend struct Scope;
	friend class SystemGroup;
};

// impl

template <typename F>
System::StageID System::stageTask(F&& func, std::vector<StageID> deps) {
	return stageTasks({.tasks = {std::forward<F>(func)}, .deps = std::move(deps)});
}

template <typename Container>
void System::waitStages(Container const& stageIDs) const {
	m_scheduler->wait_stages(stageIDs);
}
} // namespace le
