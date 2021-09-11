#pragma once
#include <string_view>
#include <core/log.hpp>
#include <core/not_null.hpp>
#include <core/time.hpp>
#include <ktl/fixed_vector.hpp>

namespace le::utils {
struct ProfileEntry {
	std::string_view name;
	time::Point stamp{};
	Time_s dt{};
};

struct ProfileDevNull {};

struct ProfileLogger {
	dl::level logLevel = dl::level::debug;

	void operator()(ProfileEntry const& entry) const { detail::logImpl(logLevel, "{:1.3f}ms [{}]", entry.dt.count() * 1000.0f, entry.name); }
};

template <std::size_t MaxEntries>
struct ProfileCacher {
	static constexpr std::size_t max_entries = MaxEntries;
	using Entries = ktl::fixed_vector<ProfileEntry, MaxEntries>;

	not_null<Entries*> entries;

	void operator()(ProfileEntry const& entry) const;
};

template <typename D>
class DProfiler {
  public:
	using dispatch_t = D;

	DProfiler(std::string_view name, D d = {}) noexcept : m_d(std::move(d)), m_name(name), m_start(time::now()) {}
	DProfiler(DProfiler&& rhs) noexcept { exchg(*this, rhs); }
	DProfiler& operator=(DProfiler rhs) noexcept { return (exchg(*this, rhs), *this); }
	~DProfiler();
	static void exchg(DProfiler& lhs, DProfiler& rhs) noexcept;

  private:
	D m_d;
	std::string_view m_name;
	time::Point m_start;
};
using LogProfiler = DProfiler<ProfileLogger>;

template <>
class DProfiler<ProfileDevNull> {
  public:
	using dispatch_t = ProfileDevNull;

	DProfiler(std::string_view, ProfileDevNull = {}) noexcept {}
	~DProfiler() noexcept {}
};
using NullProfiler = DProfiler<ProfileDevNull>;

template <std::size_t MaxEntries = 16, std::size_t RecordSize = 64>
class ProfileDB {
  public:
	static constexpr std::size_t max_entries = MaxEntries;
	static constexpr std::size_t record_size = RecordSize;

	using Cacher = ProfileCacher<MaxEntries>;
	using Profiler = DProfiler<Cacher>;
	using Entries = typename Cacher::Entries;

	struct Record {
		Entries entries;
		Time_s total{};
	};

	Profiler profile(std::string_view name) noexcept { return Profiler(name, Cacher{&m_active}); }
	void next(Time_s totalTime) {
		if (m_record.has_space()) { m_record.push_back({m_active, totalTime}); }
		m_active.clear();
	}

	ktl::fixed_vector<Record, RecordSize> m_record;

  private:
	Entries m_active;
};

struct NullProfileDB {
	using Profiler = DProfiler<ProfileDevNull>;

	Profiler profile(std::string_view name) noexcept { return Profiler(name); }
	// void next(Time_s) noexcept {}
};

// impl

template <std::size_t MaxEntries>
void ProfileCacher<MaxEntries>::operator()(ProfileEntry const& entry) const {
	auto it = std::find_if(entries->begin(), entries->end(), [n = entry.name](ProfileEntry const& entry) { return entry.name == n; });
	if (it != entries->end()) {
		it->dt += entry.dt;
		it->stamp = entry.stamp;
	} else if (entries->has_space()) {
		entries->push_back(entry);
	}
}

template <typename D>
DProfiler<D>::~DProfiler() {
	if (!m_name.empty()) {
		ProfileEntry entry{m_name, time::now(), {}};
		entry.dt = time::diff(m_start, entry.stamp);
		m_d(entry);
	}
}

template <typename D>
void DProfiler<D>::exchg(DProfiler<D>& lhs, DProfiler<D>& rhs) noexcept {
	std::swap(lhs.m_name, rhs.m_name);
	std::swap(lhs.m_d, rhs.m_d);
	std::swap(lhs.m_start, rhs.m_start);
}
} // namespace le::utils
