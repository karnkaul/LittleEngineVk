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

	inline static std::optional<std::unordered_map<Hash, Data>> s_record;

	Data data;
	std::optional<io::Level> level;
	Time start;
	Time end;
	Time min;

	explicit Profiler(std::string_view id, std::optional<io::Level> level = io::Level::eInfo, Time min = 0s);
	~Profiler();

	void stamp(std::string_view nextID = {});

	static void clear();
};
} // namespace le
