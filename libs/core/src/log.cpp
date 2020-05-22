#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include "core/log.hpp"
#include <fmt/chrono.h>
#if defined(LEVK_RUNTIME_MSVC)
#include <Windows.h>
#endif
#include "core/assert.hpp"
#include "core/std_types.hpp"
#include "core/threads.hpp"

namespace le
{
namespace
{
class FileLogger final
{
public:
	static constexpr size_t s_reserveCount = 1024 * 1024;

public:
	~FileLogger();

public:
	std::string m_cache;
	HThread m_hThread;
	std::atomic<bool> m_bLog;
	std::mutex m_mutex;

public:
	void record(std::string line);
	void dumpToFile(std::filesystem::path const& path);
	void startLogging(std::filesystem::path path, Time pollRate);
	void stopLogging();
};

FileLogger::~FileLogger()
{
	ASSERT(m_hThread == HThread::s_null, "FileLogger thread running past main!");
	if (m_hThread != HThread::s_null)
	{
		stopLogging();
	}
}

void FileLogger::startLogging(std::filesystem::path path, Time pollRate)
{
	m_cache.reserve(s_reserveCount);
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
		LOG_I("Logging to file: {}", std::filesystem::absolute(path).generic_string());
		while (m_bLog.load(std::memory_order_relaxed))
		{
			dumpToFile(path);
			if (pollRate.to_ms() <= 0)
			{
				std::this_thread::yield();
			}
			else
			{
				threads::sleep(pollRate);
			}
		}
		LOG_I("File Logging terminated");
		dumpToFile(path);
	});
	return;
}

void FileLogger::stopLogging()
{
	m_bLog.store(false);
	threads::join(m_hThread);
	m_cache.clear();
	return;
}

void FileLogger::record(std::string line)
{
	if (m_hThread != HThread::s_null)
	{
		std::scoped_lock<std::mutex> lock(m_mutex);
		m_cache += std::move(line);
		m_cache += "\n";
	}
	return;
}

void FileLogger::dumpToFile(std::filesystem::path const& path)
{
	std::string temp;
	{
		std::scoped_lock<std::mutex> lock(m_mutex);
		temp = std::move(m_cache);
		m_cache.clear();
		m_cache.reserve(s_reserveCount);
	}
	if (!temp.empty())
	{
		std::ofstream file(path, std::ios_base::app);
		file.write(temp.data(), (std::streamsize)temp.length());
	}
	return;
}

std::mutex g_logMutex;
std::array<char, (size_t)log::Level::eCOUNT_> g_prefixes = {'D', 'I', 'W', 'E'};
FileLogger g_fileLogger;
} // namespace

void log::logText([[maybe_unused]] Level level, std::string text, [[maybe_unused]] std::string_view file, [[maybe_unused]] u64 line)
{
	if ((u8)level < (u8)g_minLevel)
	{
		return;
	}
	std::time_t now = stdch::system_clock::to_time_t(stdch::system_clock::now());
	std::unique_lock<std::mutex> lock(g_logMutex);
	std::string str;
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
	try
#endif
	{
		str = fmt::format("[{}] [T{}] {} [{:%H:%M:%S}]", g_prefixes.at(size_t(level)), threads::thisThreadID(), std::move(text),
						  *std::localtime(&now));
	}
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
	catch (std::exception const& e)
	{
		ASSERT(false, e.what());
	}
#endif
	lock.unlock();
	if constexpr (g_log_bSourceLocation)
	{
		static std::string_view const parentStr = "../";
		auto const fileName = std::filesystem::path(file).generic_string();
		std::string_view fileStr(fileName);
		for (auto search = fileStr.find(parentStr); search == 0; search = fileStr.find(parentStr))
		{
			fileStr = fileStr.substr(search + parentStr.length());
		}
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
		try
#endif
		{
			str = fmt::format("{} [{}:{}]", std::move(str), fileStr, line);
		}
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
		catch (std::exception const& e)
		{
			ASSERT(false, e.what());
		}
#endif
	}
	lock.lock();
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
	try
#endif
	{
		fmt::print(stdout, "{} \n", str);
	}
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
	catch (std::exception const& e)
	{
		ASSERT(false, e.what());
	}
#endif
#if defined(LEVK_RUNTIME_MSVC)
	OutputDebugStringA(str.data());
	OutputDebugStringA("\n");
#endif
	g_fileLogger.record(std::move(str));
}

log::Service::Service(std::filesystem::path const& path, Time pollRate)
{
	logToFile(path, pollRate);
}

log::Service::~Service()
{
	stopFileLogging();
}

void log::logToFile(std::filesystem::path path, Time pollRate)
{
	if (!g_fileLogger.m_bLog.load())
	{
		g_fileLogger.startLogging(std::move(path), pollRate);
	}
	return;
}

void log::stopFileLogging()
{
	if (g_fileLogger.m_bLog.load())
	{
		g_fileLogger.stopLogging();
	}
}
} // namespace le
