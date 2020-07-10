#include <algorithm>
#include <string>
#include <sstream>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <job_manager.hpp>
#include <job_worker.hpp>

namespace le::jobs
{
Manager::Job::Job() = default;

Manager::Job::Job(s64 id, Task task, std::string name, bool bSilent) : m_task(std::move(task)), m_bSilent(bSilent)
{
	m_logName += std::to_string(id);
	if (!name.empty())
	{
		m_logName += "-";
		m_logName += std::move(name);
	}
	m_shJob = std::make_shared<Handle>(id, m_task.get_future());
}

void Manager::Job::run()
{
	m_task();
}

Manager::Manager(u8 workerCount)
{
	Worker::s_bWork.store(true, std::memory_order_seq_cst);
	for (u8 i = 0; i < workerCount; ++i)
	{
		m_jobWorkers.push_back(std::make_unique<Worker>(*this, i));
	}
}

Manager::~Manager()
{
	Worker::s_bWork.store(false, std::memory_order_seq_cst);
	// Wake all sleeping workers
	m_wakeCV.notify_all();
	// Join all worker threads
	m_jobWorkers.clear();
}

std::shared_ptr<Handle> Manager::enqueue(Task task, std::string name, bool bSilent)
{
	std::shared_ptr<Handle> ret;
	{
		auto lock = m_wakeMutex.lock();
		m_jobQueue.emplace(++m_nextJobID, std::move(task), std::move(name), bSilent);
		ret = m_jobQueue.back().m_shJob;
	}
	// Wake a sleeping worker
	m_wakeCV.notify_one();
	return ret;
}

std::vector<std::shared_ptr<Handle>> Manager::forEach(IndexedTask indexedTask)
{
	std::size_t idx = indexedTask.startIdx;
	std::vector<std::shared_ptr<Handle>> handles;
	u16 buckets = u16(indexedTask.iterationCount / indexedTask.iterationsPerJob);
	for (u16 bucket = 0; bucket < buckets; ++bucket)
	{
		std::size_t start = idx;
		std::size_t end = start + indexedTask.iterationsPerJob;
		end = end < start ? start : end > indexedTask.iterationCount ? indexedTask.iterationCount : end;
		std::string taskName;
		if (!indexedTask.bSilent)
		{
			std::stringstream name;
			name << indexedTask.name << start << "-" << (end - 1);
			taskName = name.str();
		}
		handles.push_back(enqueue(
			[start, end, indexedTask]() -> std::any {
				for (std::size_t i = start; i < end; ++i)
				{
					indexedTask.task(i);
				}
				return {};
			},
			taskName, indexedTask.bSilent));
		idx += indexedTask.iterationsPerJob;
	}
	if (idx < indexedTask.iterationCount)
	{
		std::size_t start = idx;
		std::size_t end = indexedTask.iterationCount;
		std::string taskName;
		if (!indexedTask.bSilent)
		{
			std::stringstream name;
			name << indexedTask.name << start << "-" << (end - 1);
			taskName = name.str();
		}
		handles.push_back(enqueue(
			[start, end, indexedTask]() -> std::any {
				for (std::size_t i = start; i < end; ++i)
				{
					indexedTask.task(i);
				}
				return {};
			},
			taskName, indexedTask.bSilent));
	}
	return handles;
}

bool Manager::isIdle() const
{
	for (auto& worker : m_jobWorkers)
	{
		if (worker->m_state.load() == Worker::State::eBusy)
		{
			return false;
		}
	}
	auto lock = m_wakeMutex.lock();
	return m_jobQueue.empty();
}

u8 Manager::workerCount() const
{
	return (u8)m_jobWorkers.size();
}

void Manager::waitIdle()
{
	while (!isIdle())
	{
		std::this_thread::yield();
	}
	ASSERT(isIdle(), "Workers should be idle!");
	return;
}
} // namespace le::jobs
