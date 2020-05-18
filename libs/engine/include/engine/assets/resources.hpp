#pragma once
#include <atomic>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include "core/assert.hpp"
#include "core/io.hpp"
#include "core/map_store.hpp"
#include "asset.hpp"

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

class Resources final
{
private:
	TMapStore<std::unordered_map<std::string, std::unique_ptr<Asset>>> m_resources;
	std::atomic_bool m_bActive;

public:
	static Resources& inst();

private:
	Resources();
	~Resources();

public:
	void init(IOReader const& data);

	template <typename T>
	T* create(stdfs::path const& id, typename T::Info info)
	{
		static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
		if (!m_bActive.load())
		{
			return nullptr;
		}
		ASSERT(!m_resources.find(id.generic_string()).bResult, "ID already loaded!");
		auto uT = std::make_unique<T>(id, std::move(info));
		T* pT = nullptr;
		if (uT && uT->m_status != Asset::Status::eError)
		{
			uT->setup();
			pT = uT.get();
			m_resources.emplace(id.generic_string(), std::move(uT));
		}
		return pT;
	}

	template <typename T>
	T* get(stdfs::path const& id)
	{
		static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
		if (!m_bActive.load())
		{
			return nullptr;
		}
		auto [pT, bResult] = m_resources.find(id.generic_string());
		if (bResult && pT)
		{
			return dynamic_cast<T*>(pT->get());
		}
		return nullptr;
	}

	template <typename T>
	bool unload(stdfs::path const& id)
	{
		static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset!");
		if (!m_bActive.load())
		{
			return false;
		}
		return m_resources.unload(id.generic_string());
	}

	void deinit();

	void update();
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
