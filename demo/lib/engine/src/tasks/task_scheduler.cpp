#include <algorithm>
#include <core/utils.hpp>
#include <engine/config.hpp>
#include <engine/tasks/task_scheduler.hpp>

namespace le {
TaskScheduler::TaskScheduler(u8 workerCount, bool bCatchRuntimeErrors) : TaskQueue(workerCount, bCatchRuntimeErrors) {
	m_bWork.store(true);
	m_nextID.store(0);
	m_thread = threads::newThread([this]() {
		while (m_bWork.load()) {
			threads::sleep(1ms);
			for (auto it = m_running.begin(); it != m_running.end();) {
				auto const iter = std::remove_if(it->ids.begin(), it->ids.end(), [this](TaskID id) { return status(id) >= Status::eDone; });
				it->ids.erase(iter, it->ids.end());
				if (it->ids.empty()) {
					conf::g_log.log(dl::level::info, 1, "[TaskScheduler] Stage [{}] completed", it->stage);
					m_stageStat.set(it->id.id, Status::eDone);
					it = m_running.erase(it);
				} else {
					++it;
				}
			}
			auto lock = m_mutex.lock();
			for (auto it = m_waiting.begin(); it != m_waiting.end();) {
				auto const iter = std::remove_if(it->deps.begin(), it->deps.end(), [this](StageID id) { return m_stageStat.get(id.id) >= Status::eDone; });
				it->deps.erase(iter, it->deps.end());
				if (it->deps.empty()) {
					m_stageStat.set(it->id.id, Status::eExecuting);
					for (auto const& task : it->tasks) {
						it->ids.push_back(enqueue(task));
					}
					it->tasks.clear();
					m_running.push_back(std::move(*it));
					it = m_waiting.erase(it);
				} else {
					++it;
				}
			}
		}
	});
}

TaskScheduler::~TaskScheduler() {
	m_bWork.store(false);
	m_thread = {};
}

TaskScheduler::StageID TaskScheduler::stage(Stage&& stage) {
	Entry entry;
	entry.id.id = ++m_nextID;
	entry.tasks = std::move(stage.tasks);
	entry.deps = std::move(stage.deps);
	entry.stage = std::move(stage.name);
	StageID const ret = entry.id;
	m_stageStat.set(ret.id, Status::eEnqueued);
	auto lock = m_mutex.lock();
	m_waiting.push_back(std::move(entry));
	return ret;
}

TaskScheduler::StageID TaskScheduler::stage(Stage const& stage) {
	return this->stage(Stage(stage));
}

TaskScheduler::Status TaskScheduler::stageStatus(StageID id) const {
	if (id.id == 0 || id.id > m_nextID.load()) {
		return Status::eUnknown;
	}
	return m_stageStat.get(id.id);
}

bool TaskScheduler::wait(StageID id) {
	if (id.id == 0 || id.id > m_nextID.load()) {
		return false;
	}
	return m_stageStat.wait(id.id);
}
} // namespace le
