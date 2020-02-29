#include <optional>
#include <sstream>
#include "core/jobs.hpp"
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "core/threads.hpp"
#include "jobs/job_manager.hpp"

namespace le
{
namespace
{
std::unique_ptr<JobManager> uManager;

std::shared_ptr<HJob> doNow(std::packaged_task<std::any()> task, std::optional<std::string> oName)
{
	std::string name = oName ? *oName : "unnamed";
	LOG_E("[%s] Not initialised! Running [%s] Task on this thread!", utils::tName<JobManager>().data(), name.data());
	std::shared_ptr<HJob> ret = std::make_shared<HJob>(-1, task.get_future());
	task();
	if (oName)
	{
		LOG_D("NOWORKER Completed [%s]", oName->data());
	}
	return ret;
}
} // namespace

namespace jobs
{
JobManager* g_pJobManager = nullptr;
} // namespace jobs

void jobs::init(u32 workerCount)
{
	if (uManager)
	{
		LOG_W("[%s] Already initialised ([%u] workers)!", utils::tName<JobManager>().data(), uManager->workerCount());
		return;
	}
	workerCount = std::min(workerCount, threads::maxHardwareThreads());
	uManager = std::make_unique<JobManager>(workerCount);
	g_pJobManager = uManager.get();
	LOG_D("[%s] Spawned [%u] JobWorkers ([%u] hardware threads)", utils::tName<JobManager>().data(), workerCount,
		  threads::maxHardwareThreads());
	return;
}

void jobs::cleanup()
{
	LOGIF_D(uManager, "[%s] Cleaned up (destroyed [%u] JobWorkers)", utils::tName<JobManager>().data(), uManager->workerCount());
	uManager = nullptr;
	g_pJobManager = nullptr;
	return;
}

std::shared_ptr<HJob> jobs::enqueue(std::function<std::any()> task, std::string name /* = "" */, bool bSilent /* = false */)
{
	if (uManager)
	{
		return uManager->enqueue(std::move(task), name, bSilent);
	}
	else
	{
		return doNow(std::packaged_task<std::any()>(std::move(task)), bSilent ? std::nullopt : std::optional<std::string>(name));
	}
}

std::shared_ptr<HJob> jobs::enqueue(std::function<void()> task, std::string name /* = "" */, bool bSilent /* = false */)
{
	auto doTask = [task]() -> std::any {
		task();
		return {};
	};
	if (uManager)
	{
		return uManager->enqueue(doTask, name, bSilent);
	}
	else
	{
		return doNow(std::packaged_task<std::any()>(doTask), bSilent ? std::nullopt : std::optional<std::string>(name));
	}
}

JobCatalog* jobs::createCatalogue(std::string name)
{
	ASSERT(uManager, "JobManager is null!");
	if (uManager)
	{
		return uManager->createCatalogue(std::move(name));
	}
	else
	{
		LOG_E("[%s] Not initialised! Cannot requisition new JobCatalog!", utils::tName<JobManager>().data());
		return nullptr;
	}
}

std::vector<std::shared_ptr<HJob>> jobs::forEach(IndexedTask const& indexedTask)
{
	if (uManager)
	{
		return uManager->forEach(indexedTask);
	}
	else
	{
		for (size_t startIdx = indexedTask.startIdx; startIdx < indexedTask.iterationCount * indexedTask.iterationsPerJob; ++startIdx)
		{
			std::optional<std::string> oName;
			if (!indexedTask.bSilent)
			{
				std::stringstream name;
				name << indexedTask.name << startIdx << "-" << (startIdx + indexedTask.iterationsPerJob - 1);
				oName = name.str();
			}
			doNow(std::packaged_task<std::any()>([&indexedTask, startIdx]() -> std::any {
					  indexedTask.task(startIdx);
					  return {};
				  }),
				  oName);
		}
	}
	return {};
}

void jobs::waitAll(std::vector<std::shared_ptr<HJob>> const& handles)
{
	for (auto& handle : handles)
	{
		if (handle)
		{
			handle->wait();
		}
	}
	return;
}

void jobs::update()
{
	if (uManager)
	{
		uManager->update();
	}
	return;
}

bool jobs::areWorkersIdle()
{
	return uManager ? uManager->areWorkersIdle() : true;
}

void jobs::waitForIdle()
{
	if (!areWorkersIdle())
	{
		LOG_I("[%s] Waiting for workers...", utils::tName<JobManager>().data());
	}
	while (!areWorkersIdle())
	{
		std::this_thread::yield();
	}
}
} // namespace le
