#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <core/assert.hpp>
#include <core/atomic_counter.hpp>
#include <core/hash.hpp>
#include <core/io.hpp>
#include <core/utils.hpp>
#include <core/map_store.hpp>
#include <engine/assets/asset.hpp>

namespace le
{
///
/// \brief "Component" wrapper for Asset types
///
template <typename T>
struct TAsset
{
	static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");

	///
	/// \brief Asset ID
	///
	Hash id;

	TAsset() = default;
	TAsset(stdfs::path id) : id(id) {}

	///
	/// \brief Obtain Asset pointed to by id
	/// \returns Asset*, if loaded in Resources, else `nullptr`
	///
	T* get();
	///
	/// \brief Obtain Asset pointed to by id
	/// \returns Asset*, if loaded in Resources, else `nullptr`
	///
	T const* get() const;
};

///
/// \brief Singleton for managing all classes derived from Asset
///
/// Important: Resources is a static singleton, it does not participate in RAII, and requires explicit calls to `init()` / `deinit()`.
///
class Resources final
{
public:
	using Semaphore = Counter<s32>::Semaphore;

private:
	TMapStore<std::unordered_map<Hash, std::unique_ptr<Asset>>> m_resources;
	mutable Counter<s32> m_counter;
	mutable Lockable<std::mutex> m_mutex;
	std::atomic_bool m_bActive;

public:
	static std::string const s_tName;

public:
	///
	/// \brief Reference to Resources singleton instance
	///
	static Resources& inst();

private:
	Resources();
	~Resources();

public:
	///
	/// \brief Initialise singleton and turn on service
	///
	bool init(IOReader const& data);

	///
	/// \brief Construct and store an Asset
	///
	/// Construction of Assets will occur in parallel, but insertion into map is mutex locked
	///
	template <typename T>
	T* create(stdfs::path const& id, typename T::Info info);
	///
	/// \brief Obtain loaded asset
	/// \returns Asset*, if loaded, else `nullptr`
	///
	template <typename T>
	T* get(Hash hash) const;
	///
	/// \brief Unloaded a loaded asset
	/// \returns `true` if unloaded, `false` if not present
	///
	template <typename T>
	bool unload(Hash hash);

	///
	/// \brief Unloaded a loaded asset
	/// \returns `true` if unloaded, `false` if not present
	///
	bool unload(Hash hash);
	///
	/// \brief Update all loaded Assets
	///
	void update();
	///
	/// \brief Unload all assets and turn off service
	///
	void deinit();

	///
	/// \brief Acquire busy semaphore
	/// \returns Handle to owned Semaphore
	///
	[[nodiscard]] Semaphore setBusy() const;
	///
	/// \brief Wait until owned semaphore is idle
	///
	void waitIdle();

#if defined(LEVK_EDITOR)
	template <typename T>
	std::vector<T*> loaded() const;
#endif
};

template <typename T>
T* Resources::create(stdfs::path const& id, typename T::Info info)
{
	static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
	ASSERT(m_bActive.load(), "Resources inactive!");
	T* pT = nullptr;
	if (m_bActive.load())
	{
		auto const hash = Hash(id.generic_string());
		auto semaphore = setBusy();
		auto lock = m_mutex.lock<std::unique_lock>();
		bool const bLoaded = m_resources.find(hash).bResult;
		lock.unlock();
		ASSERT(!bLoaded, "ID already loaded!");
		if (bLoaded)
		{
			pT = get<T>(id);
		}
		else
		{
			auto uT = std::make_unique<T>(id, std::move(info));
			if (uT && uT->m_status != Asset::Status::eError)
			{
				uT->setup();
				pT = uT.get();
				lock.lock();
				m_resources.emplace(hash, std::move(uT));
			}
		}
	}
	return pT;
}

template <typename T>
T* Resources::get(Hash hash) const
{
	static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
	ASSERT(m_bActive.load(), "Resources inactive!");
	if (m_bActive.load())
	{
		auto lock = m_mutex.lock();
		auto [pT, bResult] = m_resources.find(hash);
		if (bResult && pT)
		{
			return dynamic_cast<T*>(pT->get());
		}
	}
	return nullptr;
}

template <typename T>
bool Resources::unload(Hash hash)
{
	static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
	ASSERT(m_bActive.load(), "Resources inactive!");
	if (m_bActive.load())
	{
		auto lock = m_mutex.lock();
		return m_resources.unload(hash);
	}
	return false;
}

#if defined(LEVK_EDITOR)
template <typename T>
std::vector<T*> Resources::loaded() const
{
	auto semaphore = setBusy();
	static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
	ASSERT(m_bActive.load(), "Resources inactive!");
	std::vector<T*> ret;
	auto lock = m_mutex.lock();
	for (auto& [id, uT] : m_resources.m_map)
	{
		if (auto pT = dynamic_cast<T*>(uT.get()))
		{
			ret.push_back(pT);
		}
	}
	return ret;
}
#endif

template <typename T>
T* TAsset<T>::get()
{
	if (id > 0)
	{
		auto pAsset = Resources::inst().get<T>(id);
		if (pAsset && pAsset->isReady())
		{
			return dynamic_cast<T*>(pAsset);
		}
	}
	return nullptr;
}

template <typename T>
T const* TAsset<T>::get() const
{
	if (id > 0)
	{
		auto pAsset = Resources::inst().get<T>(id);
		if (pAsset && pAsset->isReady())
		{
			return dynamic_cast<T*>(pAsset);
		}
	}
	return nullptr;
}
} // namespace le
