#include <core/job_handle.hpp>
#include <core/log.hpp>
#include <core/utils.hpp>

namespace le::jobs
{
Handle::Handle(s64 jobID, std::future<std::any>&& future) noexcept : m_future(std::move(future)), m_status(Status::eWaiting), m_jobID(jobID) {}

Handle::~Handle() = default;

s64 Handle::ID() const
{
	return m_jobID;
}

std::any Handle::wait()
{
	if (m_future.valid())
	{
		return m_future.get();
	}
	static const std::any fail = false;
	return fail;
}

bool Handle::hasCompleted() const
{
	return m_status.load() == Status::eDone;
}

bool Handle::isReady() const
{
	return utils::isReady(m_future);
}

bool Handle::discard()
{
	if (m_status.load() == Status::eWaiting)
	{
		m_jobID.store(-1);
		return true;
	}
	return false;
}
} // namespace le::jobs
