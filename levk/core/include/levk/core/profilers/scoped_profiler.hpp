#pragma once
#include <levk/core/time.hpp>
#include <string_view>

namespace le {
template <typename Id>
struct ProfilerEntry {
	Id id;
	Time_s dt;
};

template <typename Id, typename Storage>
struct ProfilerEntryPusher {
	Storage& storage;
	void operator()(ProfilerEntry<Id> entry) const { storage(std::move(entry)); }
};

template <typename Dispatch, typename Id = std::string_view>
class ScopedProfiler {
  public:
	constexpr ScopedProfiler(Id id, Dispatch dispatch = {}) : m_id(std::move(id)), m_dispatch(std::move(dispatch)) {}
	constexpr ~ScopedProfiler() { lap(); }

	constexpr void lap() { m_dispatch({m_id, time::diffExchg(m_start)}); }

  private:
	Id m_id;
	Dispatch m_dispatch;
	time::Point m_start = time::now();
};
} // namespace le
