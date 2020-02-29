#pragma once
#include <string>
#include "core/log.hpp"
#include "core/time.hpp"

namespace le
{
struct Profiler
{
	std::string id;
	LogLevel level = LogLevel::Debug;
	Time dt;

	explicit Profiler(std::string_view id, LogLevel level = LogLevel::Debug);
	virtual ~Profiler();
};
} // namespace le
