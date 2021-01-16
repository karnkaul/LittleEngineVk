#pragma once
#include <memory>
#include <shared_mutex>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>
#include <core/log.hpp>
#include <engine/assets/asset_loaders.hpp>
#include <engine/config.hpp>
#include <kt/async_queue/async_queue.hpp>

namespace le {
namespace detail {
class AssetMap;
using AssetMapPtr = std::unique_ptr<AssetMap>;
template <typename T>
class TAssetMap;

struct TAssets final {
	template <typename Value>
	TAssetMap<Value>& get();
	template <typename Value>
	TAssetMap<Value> const& get() const;
	template <typename Value>
	bool contains() const noexcept;
	template <typename Value>
	std::size_t hash() const;

	std::unordered_map<std::size_t, AssetMapPtr> storeMap;
};
} // namespace detail

template <typename T>
struct Asset;

class AssetStore : public NoCopy {
  public:
	template <typename T>
	Asset<T> add(io::Path const& id, T&& t);
	template <typename T, typename Data = AssetLoadData<T>>
	std::optional<Asset<T>> load(io::Path const& id, Data&& data);
	template <typename T>
	std::optional<Asset<T>> find(Hash id);
	template <typename T>
	std::optional<Asset<T const>> find(Hash id) const;
	template <typename T>
	T& get(Hash id);
	template <typename T>
	T const& get(Hash id) const;
	template <typename T>
	bool contains(Hash id) const noexcept;
	template <typename T>
	bool unload(Hash id);

	void update();
	void clear();

	Resources& resources();

  private:
	template <typename T>
	friend class detail::TAssetMap;

	using Lock = kt::lockable<std::shared_mutex>;

	detail::TAssets m_assets;
	Resources m_resources;
	mutable Lock m_mutex;
};

template <typename T>
struct Asset {
	using OnModified = Delegate<>;

	Ref<T> t;
	Ref<OnModified> onModified;
};

template <typename T>
struct Asset<T const> {
	using OnModified = Delegate<>;

	Ref<T const> t;
	Ref<OnModified> onModified;
};

// impl
namespace detail {
template <typename T>
struct TAsset {
	using OnMod = typename Asset<T>::OnModified;

	std::string id;
	std::optional<T> t;
	std::optional<AssetLoadInfo<T>> loadInfo;
	mutable OnMod onModified;
};

class AssetMap {
  public:
	virtual ~AssetMap() = default;
	virtual void update() = 0;
};

template <typename T>
class TAssetMap : public AssetMap {
  public:
	template <typename U>
	Asset<T> add(io::Path const& id, U&& u);
	template <typename Data>
	std::optional<Asset<T>> load(AssetStore const& store, Resources& res, io::Path const& id, Data&& data);
	bool unload(Hash id);
	void update() override;

	using Storage = std::unordered_map<Hash, TAsset<T>>;
	Storage m_storage;
};

template <typename M, typename K>
constexpr bool mapContains(M&& map, K&& key) noexcept {
	return map.find(key) != map.end();
}

template <typename T, typename U>
constexpr Asset<T> makeAsset(U&& wrap) noexcept {
	return {*wrap.t, wrap.onModified};
}

template <typename T>
template <typename U>
Asset<T> TAssetMap<T>::add(io::Path const& id, U&& u) {
	auto idStr = id.generic_string();
	TAsset<T>& asset = m_storage[idStr];
	asset.t.emplace(std::forward<U>(u));
	asset.loadInfo.reset();
	conf::g_log.log(dl::level::info, 1, "== [Asset] [{}] added", idStr);
	if (asset.id.empty()) {
		asset.id = std::move(idStr);
	}
	return makeAsset<T>(asset);
}
template <typename T>
template <typename Data>
std::optional<Asset<T>> TAssetMap<T>::load(AssetStore const& store, Resources& res, io::Path const& id, Data&& data) {
	auto idStr = id.generic_string();
	AssetLoader<T> loader;
	// Store calls this without any locks, so obtain unique lock while (potentially) modifying m_storage
	auto lock = store.m_mutex.lock<std::unique_lock>();
	TAsset<T>& asset = m_storage[idStr];
	// Unique lock not needed anymore, and loader.load() may invoke store.find() etc which would need shared locks
	lock.unlock();
	asset.loadInfo = AssetLoadInfo<T>(store, res, asset.onModified, std::forward<Data>(data));
	asset.t = loader.load(*asset.loadInfo);
	if (asset.t) {
		conf::g_log.log(dl::level::info, 1, "== [Asset] [{}] loaded", idStr);
		if (asset.id.empty()) {
			asset.id = std::move(idStr);
		}
		return makeAsset<T>(asset);
	}
	conf::g_log.log(dl::level::warning, 0, "[Asset] Failed to load [{}]!", idStr);
	return std::nullopt;
}
template <typename T>
bool TAssetMap<T>::unload(Hash id) {
	if (auto it = m_storage.find(id); it != m_storage.end()) {
		conf::g_log.log(dl::level::info, 1, "-- [Asset] [{}] unloaded", it->second.id);
		m_storage.erase(it);
		return true;
	}
	return false;
}
template <typename T>
void TAssetMap<T>::update() {
	for (auto& [_, asset] : m_storage) {
		if (asset.t && asset.loadInfo && asset.loadInfo->modified()) {
			AssetLoader<T> loader;
			if (loader.reload(*asset.t, *asset.loadInfo)) {
				conf::g_log.log(dl::level::info, 1, "== [Asset] [{}] reloaded", asset.id);
				asset.onModified();
			} else {
				conf::g_log.log(dl::level::warning, 0, "[Asset] Failed to reload [{}]!", asset.id);
			}
		}
	}
}

template <typename Value>
TAssetMap<Value>& TAssets::get() {
	auto search = storeMap.find(hash<Value>());
	if (search == storeMap.end()) {
		auto [it, bRes] = storeMap.emplace(hash<Value>(), std::make_unique<TAssetMap<Value>>());
		ENSURE(bRes, "unordred_map insertion error");
		search = it;
	}
	return static_cast<TAssetMap<Value>&>(*search->second);
}
template <typename Value>
TAssetMap<Value> const& TAssets::get() const {
	if (auto search = storeMap.find(hash<Value>()); search != storeMap.end()) {
		return static_cast<TAssetMap<Value> const&>(*search->second);
	}
	throw std::runtime_error("Requested map does not exist");
}
template <typename Value>
bool TAssets::contains() const noexcept {
	return detail::mapContains(storeMap, hash<Value>());
}
template <typename Value>
std::size_t TAssets::hash() const {
	return typeid(std::decay_t<Value>).hash_code();
}
} // namespace detail

template <typename T>
Asset<T> AssetStore::add(io::Path const& id, T&& t) {
	auto lock = m_mutex.lock<std::unique_lock>();
	return m_assets.get<T>().add(id, std::forward<T>(t));
}
template <typename T, typename Data>
std::optional<Asset<T>> AssetStore::load(io::Path const& id, Data&& data) {
	// AssetLoader may invoke find() etc which would need shared locks, so
	// cannot use unique_lock for the entire function call here
	auto lock = m_mutex.lock<std::shared_lock>();
	auto& map = m_assets.get<T>();
	lock.unlock();
	// AssetMap optimally unique_locks mutex (performs load outside lock)
	return map.load(*this, m_resources, id, std::forward<Data>(data));
}
template <typename T>
std::optional<Asset<T>> AssetStore::find(Hash id) {
	auto lock = m_mutex.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		auto& store = m_assets.get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) {
			return detail::makeAsset<T>(it->second);
		}
	}
	return std::nullopt;
}
template <typename T>
std::optional<Asset<T const>> AssetStore::find(Hash id) const {
	auto lock = m_mutex.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		auto& store = m_assets.get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) {
			return detail::makeAsset<T const>(it->second);
		}
	}
	return std::nullopt;
}
template <typename T>
T& AssetStore::get(Hash id) {
	auto lock = m_mutex.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		auto& store = m_assets.get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) {
			return *it->second.t;
		}
	}
	ENSURE(false, "Asset not found!");
	throw std::runtime_error("Asset not present");
}
template <typename T>
T const& AssetStore::get(Hash id) const {
	auto lock = m_mutex.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		auto& store = m_assets.get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) {
			return *it->second.t;
		}
	}
	ENSURE(false, "Asset not found!");
	throw std::runtime_error("Asset not present");
}
template <typename T>
bool AssetStore::contains(Hash id) const noexcept {
	auto lock = m_mutex.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		return detail::mapContains(m_assets.get<T>().m_storage, id);
	}
	return false;
}
template <typename T>
bool AssetStore::unload(Hash id) {
	auto lock = m_mutex.lock<std::unique_lock>();
	if (m_assets.contains<T>()) {
		return m_assets.get<T>().unload(id);
	}
	return false;
}
inline Resources& AssetStore::resources() {
	return m_resources;
}
} // namespace le
