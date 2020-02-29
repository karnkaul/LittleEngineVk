#include <algorithm>
#include <array>
#include <ctime>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <sstream>
#include "core/std_types.hpp"
#include "core/log.hpp"
#if _MSC_VER
#include "Windows.h"
#endif

namespace le
{
namespace
{
std::mutex g_logMutex;
std::deque<std::string> g_logCache;
std::array<char const*, (size_t)LogLevel::COUNT_> g_prefixes = {"[D] ", "[I] ", "[W] ", "[E] "};

std::tm* TM(std::time_t const& time)
{
#if _MSC_VER
	static std::tm tm;
	localtime_s(&tm, &time);
	return &tm;
#else
	return std::localtime(&time);
#endif
}

void logInternal(char const* szText, [[maybe_unused]] char const* szFile, [[maybe_unused]] u64 line, LogLevel level, va_list args)
{
	static std::array<char, 1024> s_cacheStr;
	std::stringstream logText;
	logText << g_prefixes.at((size_t)level);
	std::unique_lock<std::mutex> lock(g_logMutex);
	std::vsnprintf(s_cacheStr.data(), s_cacheStr.size(), szText, args);
	logText << s_cacheStr.data();
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	auto pTM = TM(now);
	std::snprintf(s_cacheStr.data(), s_cacheStr.size(), " [%02d:%02d:%02d]", pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
	logText << s_cacheStr.data();
	g_logMutex.unlock();
#if defined(LEVK_LOG_SOURCE_LOCATION)
	static std::string const s_parentStr = "../";
	auto const file = std::filesystem::path(szFile).generic_string();
	std::string_view fileStr(file);
	auto search = fileStr.find(s_parentStr);
	while (search == 0)
	{
		fileStr = fileStr.substr(search + s_parentStr.length());
		search = fileStr.find(s_parentStr);
	}
	logText << "[" << fileStr << "|" << line << "]";
#endif
	auto logStr = logText.str();
	g_logMutex.lock();
	std::cout << logStr << std::endl;
#if _MSC_VER
	OutputDebugStringA(logStr.data());
	OutputDebugStringA("\n");
#endif
	g_logCache.push_back(std::move(logStr));
	while (g_logCache.size() > g_logCacheSize)
	{
		g_logCache.pop_front();
	}
}
} // namespace

void log(LogLevel level, char const* szText, char const* szFile, u64 line, ...)
{
	va_list argList;
	va_start(argList, line);
	logInternal(szText, szFile, line, level, argList);
	va_end(argList);
}

std::deque<std::string> logCache()
{
	std::lock_guard<std::mutex> lock(g_logMutex);
	return std::move(g_logCache);
}
} // namespace le
