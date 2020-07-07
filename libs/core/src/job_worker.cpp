#include <job_worker.hpp>
#include <job_manager.hpp>
#include <core/assert.hpp>
#include <core/utils.hpp>
#include <core/log.hpp>

namespace le::jobs
{
std::atomic_bool Worker::s_bWork = true;

Worker::Worker(Manager& manager, u8 id) : m_pManager(&manager), id(id)
{
	m_hThread = threads::newThread([&]() { run(); });
}

Worker::~Worker()
{
	threads::join(m_hThread);
}

void Worker::run()
{
	while (s_bWork.load(std::memory_order_relaxed))
	{
		m_state.store(State::eIdle);
		std::unique_lock<std::mutex> lock(m_pManager->m_wakeMutex);
		// Sleep until notified and new job exists / exiting
		m_pManager->m_wakeCV.wait(lock, [&]() { return !m_pManager->m_jobQueue.empty() || !s_bWork.load(std::memory_order_relaxed); });
		if (!s_bWork.load(std::memory_order_relaxed))
		{
			break;
		}
		Manager::Job job;
		bool bNotify = false;
		if (!m_pManager->m_jobQueue.empty())
		{
			job = std::move(m_pManager->m_jobQueue.front());
			m_pManager->m_jobQueue.pop();
			bNotify = !m_pManager->m_jobQueue.empty();
		}
		lock.unlock();
		if (bNotify)
		{
			// Wake a sleeping worker (if queue is not empty yet)
			m_pManager->m_wakeCV.notify_one();
		}
		if (job.m_shJob->m_jobID.load() >= 0)
		{
			m_state.store(State::eBusy);
			job.m_shJob->m_status.store(Handle::Status::eBusy);
			if (!job.m_bSilent)
			{
				LOG_D("[{}{}] Starting Job [{}]", utils::tName<Worker>(), id, job.m_logName);
			}
			job.run();
			if (!job.m_bSilent && job.m_shJob->m_future.valid())
			{
				try
				{
					job.m_shJob->m_future.get();
					LOG_D("[{}{}] Completed Job [{}]", utils::tName<Worker>(), id, job.m_logName);
				}
				catch (std::exception const& e)
				{
					job.m_shJob->m_exception = e.what();
					ASSERT(false, e.what());
					LOG_E("[{}{}] Threw an exception running Job [{}]!\n\t{}", utils::tName<Worker>(), id, job.m_logName, job.m_shJob->m_exception);
				}
			}
			job.m_shJob->m_status.store(Handle::Status::eDone);
		}
	}
}
} // namespace le::jobs
