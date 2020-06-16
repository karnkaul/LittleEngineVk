#pragma once
#include <any>
#include <core/job_handle.hpp>

namespace le
{
namespace jobs
{
///
/// \brief RAII wrapper for job system init/deinit
///
struct Service final
{
	bool bFlushQueue;

	Service(u8 workerCount, bool bFlushQueue = false);
	~Service();
};

///
/// \brief Initialise job system
/// \param workerCount number of worker threads to create
///
void init(u32 workerCount);
///
/// \brief Deinitialise job system
/// \param bFlushQueue whether to execute all pending tasks
///
void deinit(bool bFlushQueue = false);

///
/// \brief Enqueue a job task that returns `std::any`
///
std::shared_ptr<HJob> enqueue(std::function<std::any()> task, std::string name = "", bool bSilent = false);
///
/// \brief Enqueue a job task
///
std::shared_ptr<HJob> enqueue(std::function<void()> task, std::string name = "", bool bSilent = false);
///
/// \brief Distribute an `IndexedTask` across all workers
///
std::vector<std::shared_ptr<HJob>> forEach(IndexedTask const& indexedTask);

///
/// \brief Block until all passed jobs are ready
///
void waitAll(std::vector<std::shared_ptr<HJob>> const& handles);

///
/// \brief Check whether any job tasks are running
///
[[nodiscard]] bool isIdle();
///
/// \brief Wait until no job tasks are running
///
void waitIdle();
} // namespace jobs
} // namespace le
