#pragma once
#include <any>
#include "jobs/job_handle.hpp"
#include "jobs/job_catalogue.hpp"

namespace le
{
namespace jobs
{
void init(u32 workerCount);
void cleanup();

[[nodiscard]] std::shared_ptr<HJob> enqueue(std::function<std::any()> task, std::string name = "", bool bSilent = false);
[[nodiscard]] std::shared_ptr<HJob> enqueue(std::function<void()> task, std::string name = "", bool bSilent = false);
JobCatalog* createCatalogue(std::string name);
std::vector<std::shared_ptr<HJob>> forEach(IndexedTask const& indexedTask);

void waitAll(std::vector<std::shared_ptr<HJob>> const& handles);

void update();
[[nodiscard]] bool areWorkersIdle();
void waitForIdle();
} // namespace jobs
} // namespace le
