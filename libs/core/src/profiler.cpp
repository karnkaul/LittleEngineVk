#include <core/log.hpp>
#include <core/profiler.hpp>

namespace le
{
Profiler::Profiler(std::string_view id, std::optional<io::Level> level, Time min) : level(level), start(Time::elapsed()), min(min)
{
	data.id = id;
}

Profiler::~Profiler()
{
	stamp();
}

void Profiler::stamp(std::string_view nextID)
{
	end = Time::elapsed();
	data.dt = end - start;
	if (data.dt > min)
	{
		if (s_record.has_value())
		{
			s_record.value()[data.id] = data;
		}
		LOGIF(level, *level, "[PROFILE] [{:.3f}ms] [{}]", data.dt.to_s() * 1000.0f, data.id);
	}
	if (!nextID.empty())
	{
		data.id = nextID;
	}
	start = Time::elapsed();
}

void Profiler::clear()
{
	if (s_record.has_value())
	{
		s_record->clear();
	}
}
} // namespace le
