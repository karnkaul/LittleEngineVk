#pragma once
#include <optional>
#include <unordered_map>
#include <string>
#include "log_config.hpp"
#include "time.hpp"

namespace le
{
struct Profiler final
{
	struct Data final
	{
		std::string id;
		Time dt;
	};

	static std::optional<std::unordered_map<std::string, Data>> s_record;

	Data data;
	Time start;
	Time end;
	std::optional<log::Level> level;

	explicit Profiler(std::string_view id, std::optional<log::Level> level = log::Level::eInfo);
	~Profiler();

	static void clear();
};
} // namespace le
