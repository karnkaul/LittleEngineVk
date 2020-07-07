#include <core/log.hpp>
#include <core/profiler.hpp>

namespace le
{
std::optional<std::unordered_map<Hash, Profiler::Data>> Profiler::s_record;

Profiler::Profiler(std::string_view id, std::optional<log::Level> level) : start(Time::elapsed()), level(level)
{
	data.id = id;
}

Profiler::~Profiler()
{
	end = Time::elapsed();
	data.dt = end - start;
	LOGIF(level, *level, "[PROFILE] [{:.3f}ms] [{}]", data.dt.to_s() * 1000.0f, data.id);
	if (s_record.has_value())
	{
		auto const id = Hash(data.id);
		s_record.value()[id] = std::move(data);
	}
}

void Profiler::clear()
{
	if (s_record.has_value())
	{
		s_record->clear();
	}
}
} // namespace le
