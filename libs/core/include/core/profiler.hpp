#pragma once
#include <string>
#include "core/log.hpp"
#include "core/time.hpp"

namespace le
{
struct Profiler
{
	std::string id;
	log::Level level = log::Level::Debug;
	Time dt;

	explicit Profiler(std::string_view id, log::Level level = log::Level::Debug);
	virtual ~Profiler();
};
} // namespace le
