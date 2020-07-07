#pragma once
#include <any>
#include <future>
#include <memory>
#include <queue>
#include <vector>
#include <core/job_handle.hpp>

namespace le::jobs
{
class Manager final
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
		std::shared_ptr<Handle> m_shJob;
		bool m_bSilent = true;

	public:
		Job();
		Job(s64 id, Task task, std::string name, bool bSilent);
		Job(Job&& rhs) = default;
		Job& operator=(Job&& rhs) = default;

		void run();
	};

private:
	std::vector<std::unique_ptr<class Worker>> m_jobWorkers;
	std::queue<Job> m_jobQueue;
	mutable std::mutex m_wakeMutex;
	std::condition_variable m_wakeCV;
	s64 m_nextJobID = 0;

public:
	Manager(u8 workerCount);
	~Manager();

public:
	std::shared_ptr<Handle> enqueue(Task task, std::string name = "", bool bSilent = false);
	std::vector<std::shared_ptr<Handle>> forEach(IndexedTask indexedTask);

	bool isIdle() const;
	u8 workerCount() const;
	void waitIdle();

private:
	friend class Worker;
};
} // namespace le::jobs
