#include <levk/core/log.hpp>
#include <levk/core/profilers/scoped_profiler.hpp>

namespace le {
template <typename Id, LogLevel LevelT>
struct ProfilerLogger {
	void operator()(ProfilerEntry<Id> const& entry) const { dlog::log(LevelT, "[{}] [{:.2f}s]", entry.id, entry.dt.count()); }
};

template <typename Id = std::string_view, LogLevel LevelT = LogLevel::info>
using LogProfiler = ScopedProfiler<ProfilerLogger<Id, LevelT>, Id>;
} // namespace le
