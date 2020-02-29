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
	LOG_D("[{}] started. Running and monitoring [{}] jobs", m_logName, m_subJobs.size());
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
			LOG_D("[{}] Job [{}] completed. [{}] jobs remaining", m_logName, id, m_pendingJobs.size());
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
		[[maybe_unused]] f32 secs = (Time::elapsed() - m_startTime).to_s();
		LOG_D("{} completed {} jobs in {:.2f}s", m_logName, m_subJobs.size(), secs);
	}
	return;
}
} // namespace le
