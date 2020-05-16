#pragma once
#include <any>
#include <future>
#include <memory>
#include <queue>
#include <vector>
#include "core/job_handle.hpp"

namespace le
{
class JobManager final
{
public:
	static constexpr s32 INVALID_ID = -1;

private:
	using Task = std::function<std::any()>;

	class Job
	{
	private:
		std::packaged_task<std::any()> m_task;

	public:
		std::string m_logName;
		std::shared_ptr<HJob> m_shJob;
		s64 m_id = -1;
		bool m_bSilent = true;

	public:
		Job();
		Job(s64 id, Task task, std::string name, bool bSilent);
		Job(Job&& rhs) = default;
		Job& operator=(Job&& rhs) = default;

		void run();
	};

private:
	std::vector<std::unique_ptr<class JobWorker>> m_jobWorkers;
	std::queue<Job> m_jobQueue;
	mutable std::mutex m_wakeMutex;
	std::condition_variable m_wakeCV;
	s64 m_nextJobID = 0;

public:
	JobManager(u8 workerCount);
	~JobManager();

public:
	std::shared_ptr<HJob> enqueue(Task task, std::string name = "", bool bSilent = false);
	std::vector<std::shared_ptr<HJob>> forEach(IndexedTask const& indexedTask);

	bool isIdle() const;
	u8 workerCount() const;
	void waitIdle();

private:
	friend class JobWorker;
};
} // namespace le
