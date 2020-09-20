#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <core/log.hpp>
#include <fmt/chrono.h>
#if defined(LEVK_RUNTIME_MSVC)
#include <Windows.h>
#endif
#include <core/assert.hpp>
#include <core/std_types.hpp>
#include <core/threads.hpp>
#include <kt/async_queue/async_queue.hpp>
#include <io_impl.hpp>

namespace le
{
namespace
{
struct FileLogger final
{
	FileLogger();

	threads::TScoped thread;
};

std::filesystem::path g_logFilePath;
kt::async_queue<std::string> g_queue;
void dumpToFile(std::filesystem::path const& path, std::string const& str);

FileLogger::FileLogger()
{
	std::ifstream iFile(g_logFilePath);
	if (iFile.good())
	{
		iFile.close();
		std::filesystem::path backup(g_logFilePath);
		backup += ".bak";
		std::filesystem::rename(g_logFilePath, backup);
	}
	std::ofstream oFile(g_logFilePath);
	if (!oFile.good())
	{
		return;
	}
	oFile.close();
	g_queue.active(true);
	thread = threads::newThread([]() {
		LOG_I("Logging to file: {}", std::filesystem::absolute(g_logFilePath).generic_string());
		while (auto str = g_queue.pop())
		{
			str->append("\n");
			dumpToFile(g_logFilePath, *str);
		}
	});
	return;
}

void dumpToFile(std::filesystem::path const& path, std::string const& str)
{
	if (!path.empty() && !str.empty())
	{
		std::ofstream file(path, std::ios_base::app);
		file.write(str.data(), (std::streamsize)str.length());
	}
	return;
}

kt::lockable<std::mutex> g_logMutex;
EnumArray<io::Level, char> g_prefixes = {'D', 'I', 'W', 'E'};
std::optional<FileLogger> g_fileLogger;
} // namespace

void io::log(Level level, std::string text, [[maybe_unused]] std::string_view file, [[maybe_unused]] u64 line)
{
	if ((u8)level < (u8)g_minLevel)
	{
		return;
	}
	std::time_t now = stdch::system_clock::to_time_t(stdch::system_clock::now());
	auto lock = g_logMutex.lock<std::unique_lock>();
	std::string str;
#if defined(LEVK_LOG_CATCH_FMT_EXCEPTIONS)
	try
#endif
	{
		str = fmt::format("[{}] [T{}] {} [{:%H:%M:%S}]", g_prefixes.at(std::size_t(level)), threads::thisThreadID(), std::move(text), *std::localtime(&now));
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
		static constexpr std::string_view parentStr = "../";
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
	if (g_onLog)
	{
		g_onLog(str, level);
	}
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
	if (g_fileLogger)
	{
		g_queue.push(std::move(str));
	}
}

io::Service::Service(std::optional<std::filesystem::path> logFilePath)
{
	if (logFilePath && !logFilePath->empty())
	{
		g_logFilePath = std::move(*logFilePath);
		g_fileLogger = FileLogger();
	}
}

io::Service::~Service()
{
	impl::deinitPhysfs();
	if (g_fileLogger)
	{
		LOG_I("File Logging terminated");
		g_queue.active(false);
		g_fileLogger.reset();
		auto residue = g_queue.clear();
		for (auto& str : residue)
		{
			dumpToFile(g_logFilePath, (str + "\n"));
		}
	}
}
} // namespace le
