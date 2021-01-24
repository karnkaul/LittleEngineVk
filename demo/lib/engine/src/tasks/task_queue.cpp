#include <core/ensure.hpp>
#include <core/log.hpp>
#include <engine/config.hpp>
#include <engine/tasks/task_queue.hpp>

namespace le {
TaskQueue::Worker::Worker(StatusMap& status, kt::async_queue<Entry>& queue, u8 id) {
	thread = threads::newThread([&status, &queue, id]() {
		conf::g_log.log(dl::level::debug, 2, "[TaskQueue] Worker thread [{}] started", id);
		while (auto entry = queue.pop()) {
			status.set(entry->first.id, Status::eExecuting);
			if (bCatch) {
				try {
					run(status, *entry);
				} catch (std::runtime_error const& e) {
					status.set(id, Status::eError);
					logE("[TaskQueue] Exception caught in task [{}]: {}", entry->first.id, e.what());
					ENSURE(false, e.what());
				}
			} else {
				run(status, *entry);
			}
		}
		conf::g_log.log(dl::level::debug, 2, "[TaskQueue] Worker thread [{}] stopped", id);
	});
}

void TaskQueue::Worker::run(StatusMap& status, Entry const& entry) {
	auto const& [id, task] = entry;
	conf::g_log.log(dl::level::debug, 2, "[TaskQueue] Starting task [{}]", id.id);
	task();
	conf::g_log.log(dl::level::debug, 2, "[TaskQueue] Completed task [{}]", id.id);
	status.set(id.id, Status::eDone);
}

TaskQueue::TaskQueue(u8 workerCount, bool bCatchRuntimeErrors) {
	ENSURE(workerCount > 0, "Worker count must be non-zero");
	Worker::bCatch = bCatchRuntimeErrors;
	m_queue.active(true);
	m_nextID.store(0);
	for (u8 i = 0; i < workerCount; ++i) {
		m_workers.push_back(Worker(m_status, m_queue, i));
	}
}

TaskQueue::~TaskQueue() {
	m_queue.active(false);
	m_workers.clear();
}

TaskID TaskQueue::enqueue(Task const& task) {
	TaskID const ret = {++m_nextID};
	m_queue.push({ret, task});
	m_status.set(ret.id, Status::eEnqueued);
	return ret;
}

TaskQueue::Status TaskQueue::status(TaskID id) const {
	if (id.id > m_nextID.load()) {
		return Status::eUnknown;
	}
	return m_status.get(id.id);
}

bool TaskQueue::wait(TaskID id) {
	if (id.id == 0 || id.id > m_nextID.load()) {
		return false;
	}
	return m_status.wait(id.id);
}
} // namespace le
