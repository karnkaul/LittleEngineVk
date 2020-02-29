#pragma once
#include <any>
#include <list>
#include <vector>
#include "core/time.hpp"
#include "job_handle.hpp"

namespace le
{
class JobCatalog
{
private:
	using Task = std::function<std::any()>;
	using SubJob = std::pair<std::string, Task>;

	std::string m_logName;
	std::vector<SubJob> m_subJobs;
	std::list<std::shared_ptr<HJob>> m_pendingJobs;
	std::list<std::shared_ptr<HJob>> m_completedJobs;
	Task m_onComplete = nullptr;
	class JobManager* m_pManager;
	Time m_startTime;
	bool m_bCompleted = false;

public:
	JobCatalog(JobManager& manager, std::string name);

	void addJob(Task job, std::string name = "");
	void startJobs(Task onComplete);
	f32 progress() const;

private:
	void update();

	friend class JobManager;
};
} // namespace le
