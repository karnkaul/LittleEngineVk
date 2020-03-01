#include "core/job_handle.hpp"
#include "core/log.hpp"

namespace le
{
HJob::HJob(s64 jobID, std::future<std::any>&& future) noexcept : m_future(std::move(future)), m_jobID(jobID) {}

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
	return !m_future.valid() || m_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}

bool HJob::isReady() const
{
	return m_future.valid() && m_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}
} // namespace le
