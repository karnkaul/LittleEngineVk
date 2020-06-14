#include <optional>
#include <sstream>
#include "core/jobs.hpp"
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "core/threads.hpp"
#include "job_manager.hpp"

namespace le
{
namespace
{
std::unique_ptr<JobManager> uManager;

std::shared_ptr<HJob> doNow(std::packaged_task<std::any()> task, std::optional<std::string> oName)
{
	std::string const& name = oName ? *oName : "unnamed";
	LOG_E("[{}] Not initialised! Running [{}] Task on this thread!", utils::tName<JobManager>(), name);
	std::shared_ptr<HJob> ret = std::make_shared<HJob>(-1, task.get_future());
	task();
	if (oName)
	{
		LOG_D("NOWORKER Completed [{}]", *oName);
	}
	return ret;
}
} // namespace

namespace jobs
{
JobManager* g_pJobManager = nullptr;
} // namespace jobs

jobs::Service::Service(u8 workerCount, bool bFlushQueue) : bFlushQueue(bFlushQueue)
{
	init(workerCount);
}

jobs::Service::~Service()
{
	deinit(bFlushQueue);
}

void jobs::init(u32 workerCount)
{
	if (uManager)
	{
		LOG_W("[{}] Already initialised ([{}] workers)!", utils::tName<JobManager>(), uManager->workerCount());
		return;
	}
	workerCount = std::min(workerCount, threads::maxHardwareThreads());
	uManager = std::make_unique<JobManager>(workerCount);
	g_pJobManager = uManager.get();
	LOG_D("[{}] Spawned [{}] JobWorkers ([{}] hardware threads)", utils::tName<JobManager>(), workerCount, threads::maxHardwareThreads());
	return;
}

void jobs::deinit(bool bFlushQueue)
{
	if (bFlushQueue && uManager)
	{
		uManager->waitIdle();
	}
	[[maybe_unused]] auto count = uManager ? uManager->workerCount() : 0;
	uManager = nullptr;
	g_pJobManager = nullptr;
	LOGIF_D(count > 0, "[{}] Cleaned up (destroyed [{}] JobWorkers)", utils::tName<JobManager>(), count);
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

bool jobs::isIdle()
{
	return uManager ? uManager->isIdle() : true;
}

void jobs::waitIdle()
{
	if (!isIdle())
	{
		LOG_I("[{}] Waiting for workers...", utils::tName<JobManager>());
		uManager->waitIdle();
		LOG_I("[{}] ... workers idle", utils::tName<JobManager>());
	}
	return;
}
} // namespace le
