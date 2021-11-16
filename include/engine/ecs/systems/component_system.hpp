#pragma once
#include <core/time.hpp>
#include <core/utils/vbase.hpp>
#include <dens/registry.hpp>
#include <dens/system.hpp>
#include <dumb_tasks/scheduler.hpp>
#include <engine/input/frame.hpp>

namespace le {
struct SystemData {
	dts::scheduler& scheduler;
	input::Frame const& frame;
	Time_s dt{};
};

class ComponentSystem : public dens::system<SystemData> {
  public:
	using Order = dens::order_t;

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
	std::vector<dts::scheduler::stage_id> m_wait;
};

// impl

template <typename F>
ComponentSystem::StageID ComponentSystem::stageTask(F&& func, std::vector<StageID> deps) {
	return stageTasks({.tasks = {std::forward<F>(func)}, .deps = std::move(deps)});
}

template <typename Container>
void ComponentSystem::waitStages(Container const& stageIDs) const {
	data().scheduler.wait_stages(stageIDs);
}

inline ComponentSystem::StageID ComponentSystem::stageTasks(Stage&& stage) {
	auto const ret = data().scheduler.stage(std::move(stage));
	m_wait.push_back(ret);
	return ret;
}
} // namespace le
