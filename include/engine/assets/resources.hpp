#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <core/assert.hpp>
#include <core/io.hpp>
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
	stdfs::path id;

	TAsset() = default;
	TAsset(stdfs::path id) : id(std::move(id)) {}

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
	using Semaphore = TSemaphore<u32>;

private:
	TMapStore<std::unordered_map<std::string, std::unique_ptr<Asset>>> m_resources;
	mutable Semaphore m_semaphore;
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
	bool init(IOReader const& data);

	///
	/// \brief Construct and store an Asset
	///
	/// Construction of Assets will occur in parallel, but insertion into map is mutex locked
	///
	template <typename T>
	T* create(stdfs::path const& id, typename T::Info info)
	{
		static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
		ASSERT(m_bActive.load(), "Resources inactive!");
		T* pT = nullptr;
		if (m_bActive.load())
		{
			auto semaphore = setBusy();
			ASSERT(!m_resources.find(id.generic_string()).bResult, "ID already loaded!");
			auto uT = std::make_unique<T>(id, std::move(info));
			if (uT && uT->m_status != Asset::Status::eError)
			{
				uT->setup();
				pT = uT.get();
				std::scoped_lock<std::mutex> lock(m_semaphore.m_mutex);
				m_resources.emplace(id.generic_string(), std::move(uT));
			}
		}
		return pT;
	}

	///
	/// \brief Obtain loaded asset
	/// \returns Asset*, if loaded, else `nullptr`
	///
	template <typename T>
	T* get(stdfs::path const& id) const
	{
		static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
		ASSERT(m_bActive.load(), "Resources inactive!");
		if (m_bActive.load())
		{
			std::scoped_lock<std::mutex> lock(m_semaphore.m_mutex);
			auto [pT, bResult] = m_resources.find(id.generic_string());
			if (bResult && pT)
			{
				return dynamic_cast<T*>(pT->get());
			}
		}
		return nullptr;
	}

	///
	/// \brief Unloaded a loaded asset
	/// \returns `true` if unloaded, `false` if not present
	///
	template <typename T>
	bool unload(stdfs::path const& id)
	{
		static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
		ASSERT(m_bActive.load(), "Resources inactive!");
		if (m_bActive.load())
		{
			std::scoped_lock<std::mutex> lock(m_semaphore.m_mutex);
			return m_resources.unload(id.generic_string());
		}
		return false;
	}

	///
	/// \brief Unloaded a loaded asset
	/// \returns `true` if unloaded, `false` if not present
	///
	bool unload(stdfs::path const& id);

	void update();
	void deinit();

	Semaphore::Handle setBusy() const;
	void waitIdle();

#if defined(LEVK_EDITOR)
	template <typename T>
	std::vector<T*> loaded() const
	{
		auto semaphore = setBusy();
		static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
		ASSERT(m_bActive.load(), "Resources inactive!");
		std::vector<T*> ret;
		std::scoped_lock<std::mutex> lock(m_semaphore.m_mutex);
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
