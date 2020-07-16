#pragma once
#include <optional>
#include <unordered_map>
#include <string>
#include <core/hash.hpp>
#include <core/log_config.hpp>
#include <core/time.hpp>

namespace le
{
struct Profiler final
{
	struct Data final
	{
		std::string id;
		Time dt;
	};

	static std::optional<std::unordered_map<Hash, Data>> s_record;

	Data data;
	Time start;
	Time end;
	std::optional<io::Level> level;

	explicit Profiler(std::string_view id, std::optional<io::Level> level = io::Level::eInfo);
	~Profiler();

	static void clear();
};
} // namespace le
