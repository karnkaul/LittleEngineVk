#pragma once
#include <ktl/fixed_vector.hpp>
#include <levk/core/profilers/scoped_profiler.hpp>

namespace le {
template <std::size_t Capacity, typename Id = std::string_view>
struct RecordProfilerStorage;

template <std::size_t Capacity, typename Id>
struct RecordProfilerStorage {
	using dispatch_type = ProfilerEntryPusher<Id, RecordProfilerStorage>;
	using profiler_type = ScopedProfiler<dispatch_type, Id>;

	struct Record {
		ktl::fixed_vector<ProfilerEntry<Id>, Capacity> entries{};
		Time_s total{};
	};

	Record record{}, active{};
	time::Point start = time::now();

	void operator()(ProfilerEntry<Id> entry) {
		if (active.entries.has_space()) { active.entries.push_back({std::move(entry)}); }
	}

	dispatch_type dispatch() { return {*this}; }
	profiler_type profile(Id id) { return {std::move(id), dispatch()}; }

	void update() {
		active.total = time::diffExchg(start);
		record = std::exchange(active, {});
	}
};
} // namespace le
