#pragma once
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <core/assert.hpp>
#include <core/io.hpp>
#include <core/map_store.hpp>
#include <engine/assets/asset.hpp>

namespace le
{
template <typename T>
struct TAsset
{
	static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");

	stdfs::path id;

	TAsset() = default;
	TAsset(stdfs::path id) : id(std::move(id)) {}

	T* get();
	T const* get() const;
};

// Resources is a static singleton, it does not participate in RAII,
// and requires explicit calls to `init()` / `deinit()`.
class Resources final
{
private:
	TMapStore<std::unordered_map<std::string, std::unique_ptr<Asset>>> m_resources;
	std::mutex m_mutex;			   // Locked for init()/deinit()
	std::shared_mutex m_semaphore; // Blocks deinit() until all API threads have returned
	std::atomic_bool m_bActive;	   // API on/off switch

public:
	static std::string const s_tName;

public:
	static Resources& inst();

private:
	Resources();
	~Resources();

public:
	bool init(IOReader const& data);

	// Construction of Assets will occur in parallel,
	// but insertion into map is mutex locked (by TMapStore)
	template <typename T>
	T* create(stdfs::path const& id, typename T::Info info)
	{
		static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
		T* pT = nullptr;
		ASSERT(m_bActive.load(), "Resources inactive!");
		if (m_bActive.load())
		{
			std::shared_lock<std::shared_mutex> lock(m_semaphore);
			ASSERT(!m_resources.find(id.generic_string()).bResult, "ID already loaded!");
			auto uT = std::make_unique<T>(id, std::move(info));
			if (uT && uT->m_status != Asset::Status::eError)
			{
				uT->setup();
				pT = uT.get();
				m_resources.emplace(id.generic_string(), std::move(uT));
			}
		}
		return pT;
	}

	template <typename T>
	T* get(stdfs::path const& id)
	{
		std::shared_lock<std::shared_mutex> lock(m_semaphore);
		static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
		ASSERT(m_bActive.load(), "Resources inactive!");
		if (m_bActive.load())
		{
			auto [pT, bResult] = m_resources.find(id.generic_string());
			if (bResult && pT)
			{
				return dynamic_cast<T*>(pT->get());
			}
		}
		return nullptr;
	}

	template <typename T>
	bool unload(stdfs::path const& id)
	{
		std::shared_lock<std::shared_mutex> lock(m_semaphore);
		static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
		ASSERT(m_bActive.load(), "Resources inactive!");
		return m_bActive.load() ? m_resources.unload(id.generic_string()) : false;
	}

	void update();
	void deinit();
};

template <typename T>
T* TAsset<T>::get()
{
	return id.empty() ? nullptr : dynamic_cast<T*>(Resources::inst().get<T>(id));
}

template <typename T>
T const* TAsset<T>::get() const
{
	return id.empty() ? nullptr : dynamic_cast<T*>(Resources::inst().get<T>(id));
}
} // namespace le
