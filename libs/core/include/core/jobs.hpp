#pragma once
#include <any>
#include "job_handle.hpp"

namespace le
{
namespace jobs
{
struct Service final
{
	bool bFlushQueue;

	Service(u8 workerCount, bool bFlushQueue = false);
	~Service();
};

void init(u32 workerCount);
void cleanup(bool bFlushQueue = false);

std::shared_ptr<HJob> enqueue(std::function<std::any()> task, std::string name = "", bool bSilent = false);
std::shared_ptr<HJob> enqueue(std::function<void()> task, std::string name = "", bool bSilent = false);
std::vector<std::shared_ptr<HJob>> forEach(IndexedTask const& indexedTask);

void waitAll(std::vector<std::shared_ptr<HJob>> const& handles);

[[nodiscard]] bool areWorkersIdle();
void waitForIdle();
} // namespace jobs
} // namespace le
