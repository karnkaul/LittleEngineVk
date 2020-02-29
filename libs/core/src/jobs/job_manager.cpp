#include <algorithm>
#include <string>
#include <sstream>
#include "core/jobs/job_catalogue.hpp"
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "job_manager.hpp"
#include "jobWorker.hpp"

namespace le
{
using Lock = std::lock_guard<std::mutex>;

JobManager::Job::Job() = default;

JobManager::Job::Job(s64 id, Task task, std::string name, bool bSilent) : m_task(std::move(task)), m_id(id), m_bSilent(bSilent)
{
	m_logName = "[";
	m_logName += std::to_string(id);
	if (!name.empty())
	{
		m_logName += "-";
		m_logName += std::move(name);
	}
	m_logName += "]";
	m_shJob = std::make_shared<HJob>(id, m_task.get_future());
}

void JobManager::Job::run()
{
	try
	{
		m_task();
	}
	catch (std::exception const& e)
	{
		ASSERT_STR(false, e.what());
		m_exception = e.what();
	}
}

JobManager::JobManager(u8 workerCount)
{
	JobWorker::s_bWork.store(true, std::memory_order_seq_cst);
	for (u8 i = 0; i < workerCount; ++i)
	{
		m_jobWorkers.push_back(std::make_unique<JobWorker>(*this, i));
	}
}

JobManager::~JobManager()
{
	JobWorker::s_bWork.store(false, std::memory_order_seq_cst);
	// Wake all sleeping workers
	m_wakeCV.notify_all();
	// Join all worker threads
	m_jobWorkers.clear();
}

std::shared_ptr<HJob> JobManager::enqueue(Task task, std::string name, bool bSilent)
{
	std::shared_ptr<HJob> ret;
	{
		Lock lock(m_wakeMutex);
		m_jobQueue.emplace(++m_nextJobID, std::move(task), std::move(name), bSilent);
		ret = m_jobQueue.back().m_shJob;
	}
	// Wake a sleeping worker
	m_wakeCV.notify_one();
	return ret;
}

JobCatalog* JobManager::createCatalogue(std::string name)
{
	m_catalogs.push_back(std::make_unique<JobCatalog>(*this, std::move(name)));
	return m_catalogs.back().get();
}

std::vector<std::shared_ptr<HJob>> JobManager::forEach(IndexedTask const& indexedTask)
{
	size_t idx = indexedTask.startIdx;
	std::vector<std::shared_ptr<HJob>> handles;
	u16 buckets = u16(indexedTask.iterationCount / indexedTask.iterationsPerJob);
	for (u16 bucket = 0; bucket < buckets; ++bucket)
	{
		size_t start = idx;
		size_t end = start + indexedTask.iterationsPerJob;
		end = end < start ? start : end > indexedTask.iterationCount ? indexedTask.iterationCount : end;
		std::string taskName;
		if (!indexedTask.bSilent)
		{
			std::stringstream name;
			name << indexedTask.name << start << "-" << (end - 1);
			taskName = name.str();
		}
		handles.push_back(enqueue(
			[start, end, &indexedTask]() -> std::any {
				for (size_t i = start; i < end; ++i)
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
		size_t start = idx;
		size_t end = indexedTask.iterationCount;
		std::string taskName;
		if (!indexedTask.bSilent)
		{
			std::stringstream name;
			name << indexedTask.name << start << "-" << (end - 1);
			taskName = name.str();
		}
		handles.push_back(enqueue(
			[start, end, &indexedTask]() -> std::any {
				for (size_t i = start; i < end; ++i)
				{
					indexedTask.task(i);
				}
				return {};
			},
			taskName, indexedTask.bSilent));
	}
	return handles;
}

void JobManager::update()
{
	auto iter = m_catalogs.begin();
	while (iter != m_catalogs.end())
	{
		auto& uCatalog = *iter;
		uCatalog->update();
		if (uCatalog->m_bCompleted)
		{
			LOG_D("[%s] [%s] JobCatalog completed. Destroying instance.", utils::tName<JobManager>().data(), uCatalog->m_logName.data());
			iter = m_catalogs.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

bool JobManager::areWorkersIdle() const
{
	Lock lock(m_wakeMutex);
	for (auto& gameWorker : m_jobWorkers)
	{
		if (gameWorker->m_state == JobWorker::State::Busy)
		{
			return false;
		}
	}
	return m_jobQueue.empty();
}

u16 JobManager::workerCount() const
{
	return (u16)m_jobWorkers.size();
}
} // namespace le
