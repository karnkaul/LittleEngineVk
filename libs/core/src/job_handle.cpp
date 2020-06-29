#include <core/job_handle.hpp>
#include <core/log.hpp>
#include <core/utils.hpp>

namespace le
{
HJob::HJob(s64 jobID, std::future<std::any>&& future) noexcept : m_future(std::move(future)), m_status(Status::eWaiting), m_jobID(jobID) {}

HJob::~HJob() = default;

s64 HJob::ID() const
{
	return m_jobID;
}

std::any HJob::wait()
{
	if (m_future.valid())
	{
		return m_future.get();
	}
	static const std::any fail = false;
	return fail;
}

bool HJob::hasCompleted() const
{
	return m_status.load() == Status::eDone;
}

bool HJob::isReady() const
{
	return utils::isReady(m_future);
}

bool HJob::discard()
{
	if (m_status.load() == Status::eWaiting)
	{
		m_jobID.store(-1);
		return true;
	}
	return false;
}
} // namespace le
