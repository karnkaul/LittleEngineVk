#include "core/profiler.hpp"

namespace le
{
Profiler::Profiler(std::string_view id, LogLevel level) : id(id), level(level), dt(Time::elapsed()) {}

Profiler::~Profiler()
{
	dt = Time::elapsed() - dt;
	LOG(level, "[Profile] [%s] [%.3fms]", id.data(), dt.to_s() * 1000.0f);
}
} // namespace le
