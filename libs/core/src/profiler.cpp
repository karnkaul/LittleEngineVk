#include "core/profiler.hpp"

namespace le
{
Profiler::Profiler(std::string_view id, log::Level level) : id(id), level(level), dt(Time::elapsed()) {}

Profiler::~Profiler()
{
	dt = Time::elapsed() - dt;
	LOG(level, "[Profile] [{}] [{:.3f}ms]", id, dt.to_s() * 1000.0f);
}
} // namespace le
