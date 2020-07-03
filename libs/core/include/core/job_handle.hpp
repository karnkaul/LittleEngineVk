#pragma once
#include <any>
#include <atomic>
#include <functional>
#include <future>
#include <string>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Handle to a scheduled/running/completed job task
///
class HJob final
{
public:
	enum class Status : s8
	{
		eIdle,
		eWaiting,
		eBusy,
		eDone,
	};

public:
	std::string m_exception;

private:
	std::future<std::any> m_future;
	std::atomic<Status> m_status;
	std::atomic<s64> m_jobID = -1;

public:
	HJob(s64 jobID, std::future<std::any>&& future) noexcept;
	~HJob();

	///
	/// \brief Obtain job ID
	///
	s64 ID() const;

	///
	/// \brief Block this thread until this job task has completed
	///
	std::any wait();
	///
	/// \brief Check whether this job task has completed / was never started
	///
	bool hasCompleted() const;
	///
	/// \brief Check whether this job task is ready
	///
	bool isReady() const;
	///
	/// \brief Discard this job (if waiting in queue)
	///
	bool discard();

private:
	friend class JobWorker;
	friend class JobManager;
};

///
/// \brief Helper struct for running indexed tasks across multiple workers
///
struct IndexedTask final
{
	std::function<void(std::size_t)> task;
	std::string name;
	std::size_t iterationCount = 0;
	std::size_t iterationsPerJob = 1;
	std::size_t startIdx = 0;
	bool bSilent = true;
};
} // namespace le
