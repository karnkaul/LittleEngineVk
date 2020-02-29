#include "core/jobs/job_catalogue.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "job_manager.hpp"

namespace le
{
JobCatalog::JobCatalog(JobManager& manager, std::string name) : m_pManager(&manager)
{
	m_logName.reserve(name.size() + 2);
	m_logName += "[";
	m_logName += std::move(name);
	m_logName += "]";
}

void JobCatalog::addJob(Task job, std::string name)
{
	if (name.empty())
	{
		name = "JobCatalog_" + std::to_string(m_subJobs.size());
	}
	m_subJobs.emplace_back(std::move(name), std::move(job));
	return;
}

void JobCatalog::startJobs(Task onComplete)
{
	LOG_D("%s started. Running and monitoring %d jobs", m_logName.c_str(), m_subJobs.size());
	m_onComplete = onComplete;
	m_bCompleted = false;
	m_startTime = Time::elapsed();
	for (auto& job : m_subJobs)
	{
		m_pendingJobs.push_back(m_pManager->enqueue(std::move(job.second), std::move(job.first)));
	}
	return;
}

f32 JobCatalog::progress() const
{
	u32 done = u32(m_subJobs.size() - m_pendingJobs.size());
	return (f32)done / m_subJobs.size();
}

void JobCatalog::update()
{
	auto iter = m_pendingJobs.begin();
	while (iter != m_pendingJobs.end())
	{
		auto const& subJob = *iter;
		if (subJob->hasCompleted())
		{
			[[maybe_unused]] auto id = subJob->ID();
			iter = m_pendingJobs.erase(iter);
#if defined(LEVK_DEBUG_LOG)
			LOG_D("%s Job %d completed. %d jobs remaining", m_logName.c_str(), id, m_pendingJobs.size());
#endif
		}
		else
		{
			++iter;
		}
	}

	if (m_pendingJobs.empty() && !m_bCompleted)
	{
		if (m_onComplete)
		{
			m_onComplete();
			m_onComplete = nullptr;
		}
		m_bCompleted = true;
#if defined(LEVK_DEBUG_LOG)
		f32 secs = (Time::elapsed() - m_startTime).to_s();
		LOG_D("%s completed %d jobs in %.2fs", m_logName.c_str(), m_subJobs.size(), secs);
#endif
	}
	return;
}
} // namespace le
