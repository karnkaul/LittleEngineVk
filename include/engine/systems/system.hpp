#pragma once
#include <core/time.hpp>
#include <core/utils/vbase.hpp>
#include <dens/registry.hpp>
#include <dumb_tasks/scheduler.hpp>

namespace le {
class SystemBase : public utils::VBase {
  protected:
	using Scheduler = dts::scheduler;
	using Stage = Scheduler::stage_t;
	using StageID = Scheduler::stage_id;

	virtual void tick(dens::registry const& registry, Time_s dt) = 0;

	template <typename F>
	StageID stageTask(F&& func, std::vector<StageID> deps = {});
	StageID stageTasks(Stage&& stage);
	template <typename Container>
	void waitStages(Container const& stageIDs) const;

  private:
	void update(dts::scheduler& scheduler, dens::registry const& registry, Time_s dt);

	std::vector<dts::scheduler::stage_id> m_wait;
	dts::scheduler* m_scheduler{};

	struct Scope;
	friend struct Scope;
	friend class Systems;
};

template <typename T>
class System;

// impl

template <typename F>
SystemBase::StageID SystemBase::stageTask(F&& func, std::vector<StageID> deps) {
	return stageTasks({.tasks = {std::forward<F>(func)}, .deps = std::move(deps)});
}

template <typename Container>
void SystemBase::waitStages(Container const& stageIDs) const {
	m_scheduler->wait_stages(stageIDs);
}
} // namespace le
