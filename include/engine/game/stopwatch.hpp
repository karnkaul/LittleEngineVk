#pragma once
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include <core/hash.hpp>
#include <core/log.hpp>
#include <core/ref.hpp>
#include <core/time.hpp>
#include <core/traits.hpp>
#include <core/utils.hpp>

namespace le
{
///
/// \brief Compile-time tag for Stopwatch
///
enum class SWTag
{
	///
	/// \brief Tag for using a mutex lock (undefined)
	///
	eMutex,
	///
	/// \brief Tag for not using a mutex lock (undefined)
	///
	eNoMutex
};

///
/// \brief Stopwatch that stores entries by name/hash
/// \param Tag One of mutex_t or no_mutex_t
///
template <SWTag Tag = SWTag::eMutex>
class Stopwatch
{
public:
	static constexpr bool hasMutex = Tag == SWTag::eMutex;

public:
	enum class Log
	{
		///
		/// \brief Don't log anything
		///
		eNone,
		///
		/// \brief Log dt for each Stamp
		///
		eGameDT,
		///
		/// \brief Log real time elapsed for each Stamp
		///
		eRealDT
	};
	///
	/// \brief One timestamp
	///
	struct Stamp final
	{
		///
		/// \brief Real time
		///
		struct
		{
			Time begin;
			Time end;
		} points;
		///
		/// \brief Game time
		///
		Time dt;
		///
		/// \brief Obtain game dt
		///
		constexpr operator Time() const noexcept;
		///
		/// \brief Obtain real dt
		///
		constexpr Time real() const noexcept;
	};
	///
	/// \brief Collection of Stamps with an associated label
	///
	struct Info final
	{
		std::string label;
		std::vector<Stamp> stamps;
		Time updated;
	};
	///
	/// \brief RAII wrapper that stops on destruction
	///
	struct Scoped final
	{
		Ref<Stopwatch<Tag>> parent;
		Hash id;
		Log log;

		Scoped(Ref<Stopwatch<Tag>> parent, Hash id, Log log);
		Scoped(Scoped&&) noexcept;
		Scoped& operator=(Scoped&&);
		~Scoped();
	};

public:
	std::string m_name;

public:
	///
	/// \brief Start a timestamp
	///
	Info const& start(std::string label, Hash customHash);
	///
	/// \brief Stop the last started timestamp
	///
	Stamp const* stop(Hash hash, Log log = Log::eNone);
	///
	/// \brief Obtain a scoped timestamp
	///
	Scoped lap(std::string label, Hash customHash = {}, Log = Log::eRealDT);

	///
	/// \brief Erase an Info
	///
	bool erase(Hash hash);
	///
	/// \brief Obtain a pointer to an Info
	///
	Info const* find(Hash hash) const;
	///
	/// \brief Obtain references to all stored Info instances, sorted by label
	///
	std::vector<Ref<Info const>> laps(bool bSort = true) const;
	///
	/// \brief Update game timestamps
	///
	void tick(Time dt);

protected:
	std::unordered_map<Hash, Info> m_infos;
	utils::Lockable<hasMutex> m_mutex;
};

///
/// \brief Global stopwatch
///
inline Stopwatch<> g_stopwatch;

template <SWTag Tag>
constexpr Stopwatch<Tag>::Stamp::operator Time() const noexcept
{
	return dt;
}

template <SWTag Tag>
constexpr Time Stopwatch<Tag>::Stamp::real() const noexcept
{
	return points.end - points.begin;
}

template <SWTag Tag>
Stopwatch<Tag>::Scoped::Scoped(Ref<Stopwatch<Tag>> parent, Hash id, Log log) : parent(parent), id(id), log(log)
{
}

template <SWTag Tag>
Stopwatch<Tag>::Scoped::Scoped(Scoped&& rhs) noexcept : parent(rhs.parent), id(rhs.id), log(rhs.log)
{
	rhs.id = {};
}

template <SWTag Tag>
typename Stopwatch<Tag>::Scoped& Stopwatch<Tag>::Scoped::operator=(Scoped&& rhs)
{
	if (&rhs != this)
	{
		if (id != Hash())
		{
			Stopwatch& p = parent;
			p.stop(id, log);
		}
		parent = rhs.parent;
		id = rhs.id;
		log = rhs.log;
		rhs.id = {};
	}
	return *this;
}

template <SWTag Tag>
Stopwatch<Tag>::Scoped::~Scoped()
{
	if (id != Hash())
	{
		Stopwatch& s = parent;
		s.stop(id, log);
	}
}

template <SWTag Tag>
typename Stopwatch<Tag>::Info const& Stopwatch<Tag>::start(std::string label, Hash customHash)
{
	auto const id = customHash == Hash() ? label : customHash;
	auto lock = m_mutex.lock();
	auto& info = m_infos[id];
	Stamp stamp;
	info.label = std::move(label);
	info.updated = stamp.points.begin = Time::elapsed();
	info.stamps.push_back(stamp);
	return info;
}

template <SWTag Tag>
typename Stopwatch<Tag>::Stamp const* Stopwatch<Tag>::stop(Hash hash, Log log)
{
	auto lock = m_mutex.lock();
	if (auto search = m_infos.find(hash); search != m_infos.end())
	{
		auto& info = search->second;
		if (!info.stamps.empty())
		{
			auto& stamp = info.stamps.back();
			info.updated = stamp.points.end = Time::elapsed();
			if (log > Log::eNone)
			{
				if (m_name.empty())
				{
					m_name = "Stopwatch";
				}
				Time const dt = log == Log::eGameDT ? stamp.dt : stamp.real();
				LOG_I("[{}] [{}] : [{:.2f}ms]", m_name, info.label, dt.to_s() * 1000);
			}
			return &stamp;
		}
	}
	return nullptr;
}

template <SWTag Tag>
typename Stopwatch<Tag>::Scoped Stopwatch<Tag>::lap(std::string label, Hash customHash, Log log)
{
	auto const id = customHash == Hash() ? label : customHash;
	start(std::move(label), id);
	return Scoped(*this, id, log);
}

template <SWTag Tag>
bool Stopwatch<Tag>::erase(Hash hash)
{
	auto lock = m_mutex.lock();
	if (auto search = m_infos.find(hash); search != m_infos.end())
	{
		m_infos.erase(search);
		return true;
	}
	return false;
}

template <SWTag Tag>
typename Stopwatch<Tag>::Info const* Stopwatch<Tag>::find(Hash hash) const
{
	auto lock = m_mutex.lock();
	if (auto search = m_infos.find(hash); search != m_infos.end())
	{
		return &search->second;
	}
	return nullptr;
}

template <SWTag Tag>
std::vector<Ref<typename Stopwatch<Tag>::Info const>> Stopwatch<Tag>::laps(bool bSort) const
{
	std::vector<Ref<Info const>> ret;
	auto lock = m_mutex.lock();
	ret.reserve(m_infos.size());
	for (auto const& [_, info] : m_infos)
	{
		ret.push_back(info);
	}
	if (bSort)
	{
		std::sort(ret.begin(), ret.end(), [](Info const& lhs, Info const& rhs) -> bool { return lhs.label < rhs.label; });
	}
	return ret;
}

template <SWTag Tag>
void Stopwatch<Tag>::tick(Time dt)
{
	auto lock = m_mutex.lock();
	for (auto& [_, info] : m_infos)
	{
		if (!info.stamps.empty())
		{
			Stamp& stamp = info.stamps.back();
			stamp.points.end = Time::elapsed();
			stamp.dt += dt;
		}
	}
}
} // namespace le
