#include <fstream>
#include <iostream>
#include <thread>
#include "core/log.hpp"
#include "core/os.hpp"
#include "core/file_logger.hpp"

namespace le
{
FileLogger::FileLogger(std::filesystem::path path, Time pollRate)
{
	m_bLog.store(true, std::memory_order_relaxed);
	std::ifstream iFile(path);
	if (iFile.good())
	{
		iFile.close();
		std::filesystem::path backup(path);
		backup += ".bak";
		std::filesystem::rename(path, backup);
	}
	std::ofstream oFile(path);
	if (!oFile.good())
	{
		return;
	}
	oFile.close();
	m_hThread = threads::newThread([this, pollRate, path]() {
		while (m_bLog.load(std::memory_order_relaxed))
		{
			dumpToFile(path);
			if (pollRate.to_ms() <= 0)
			{
				std::this_thread::yield();
			}
			else
			{
				std::this_thread::sleep_for(pollRate.usecs);
			}
		}
		LOG_I("[Log] File logging terminated");
		dumpToFile(path);
	});
}

FileLogger::~FileLogger()
{
	m_bLog.store(false);
	threads::join(m_hThread);
}

void FileLogger::dumpToFile(std::filesystem::path const& path)
{
	auto cache = logCache();
	if (!cache.empty())
	{
		std::ofstream file(path, std::ios_base::app);
		for (auto& logStr : cache)
		{
			logStr += os::g_EOL;
			file.write(logStr.data(), (std::streamsize)logStr.size());
		}
	}
	return;
}
} // namespace le
