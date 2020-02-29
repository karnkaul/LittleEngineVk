#pragma once
#include <atomic>
#include <filesystem>
#include "core/time.hpp"
#include "core/threads.hpp"

namespace le
{
class FileLogger final
{
private:
	HThread m_hThread;
	std::atomic<bool> m_bLog;

public:
	explicit FileLogger(std::filesystem::path path, Time pollRate = Time::from_s(0.5f));
	~FileLogger();

private:
	void dumpToFile(std::filesystem::path const& path);
};
} // namespace le
