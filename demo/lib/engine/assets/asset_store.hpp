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
class Asset;

class AssetStore : public NoCopy {
  public:
	using OnModified = Delegate<>;

	AssetStore() = default;
	~AssetStore();

	template <typename T>
	Asset<T> add(io::Path const& id, T&& t);
	template <typename T, typename Data = AssetLoadData<T>>
	std::optional<Asset<T>> load(io::Path const& id, Data&& data);
	template <typename T>
	std::optional<Asset<T>> find(Hash id);
	template <typename T>
	std::optional<Asset<T const>> find(Hash id) const;
	template <typename T>
	Asset<T> get(Hash id);
	template <typename T>
	Asset<T const> get(Hash id) const;
	template <typename T>
	bool contains(Hash id) const noexcept;
	template <typename T>
	bool unload(Hash id);
	template <typename T>
	bool reload(Hash id);

	void update();
	void clear();

	template <template <typename...> typename L = std::scoped_lock>
	L<std::mutex> lock() const;

	Resources& resources();

  private:
	detail::TAssets m_assets;
	Resources m_resources;
	mutable std::unordered_map<Hash, OnModified> m_onModified;
	mutable kt::lockable<std::shared_mutex> m_mrsw;
	mutable kt::lockable<std::mutex> m_mutex;

	template <typename T>
	friend class detail::TAssetMap;
};

template <typename T>
class Asset {
  public:
	using type = T;
	using OnModified = AssetStore::OnModified;

	Asset(type& t, OnModified& onMod, Hash id);

	type& get() const;
	type& operator*();
	type& operator->();
	OnModified::Tk onModified(OnModified::Callback const& callback);

	Hash m_id;

  private:
	Ref<T> m_t;
	Ref<OnModified> m_onModified;
};

template <typename T>
class Asset<T const> {
  public:
	using type = T;
	using OnModified = AssetStore::OnModified;

	Asset(type const& t, OnModified& onMod, Hash id);

	type const& get() const;
	type const& operator*();
	type const& operator->();
	OnModified::Tk onModified(OnModified::Callback const& callback);

	Hash m_id;

  private:
	Ref<T const> m_t;
	Ref<OnModified> m_onModified;
};

// impl

namespace detail {
template <typename T>
struct TAsset {
	std::string id;
	std::optional<T> t;
	std::optional<AssetLoadInfo<T>> loadInfo;
};

class AssetMap {
  public:
	virtual ~AssetMap() = default;
	virtual u64 update(AssetStore const& store) = 0;
};

template <typename T>
class TAssetMap : public AssetMap {
  public:
	template <typename U>
	Asset<T> add(AssetStore const& store, io::Path const& id, U&& u);
	template <typename Data>
	std::optional<Asset<T>> load(AssetStore const& store, Resources& res, io::Path const& id, Data&& data);
	bool reloadAsset(AssetStore const& store, TAsset<T>& out_t) const;
	bool reload(AssetStore const& store, Hash id);
	bool unload(Hash id);
	u64 update(AssetStore const& store) override;

	using Storage = std::unordered_map<Hash, TAsset<T>>;
	Storage m_storage;
};

template <typename M, typename K>
constexpr bool mapContains(M&& map, K&& key) noexcept {
	return map.find(key) != map.end();
}

template <typename T, typename U>
constexpr Asset<T> makeAsset(U&& wrap, AssetStore::OnModified& onModified) noexcept {
	return {*wrap.t, onModified, wrap.id};
}

template <typename T>
template <typename U>
Asset<T> TAssetMap<T>::add(AssetStore const& store, io::Path const& id, U&& u) {
	auto idStr = id.generic_string();
	TAsset<T>& asset = m_storage[idStr];
	asset.t.emplace(std::forward<U>(u));
	asset.loadInfo.reset();
	conf::g_log.log(dl::level::info, 1, "== [Asset] [{}] added", idStr);
	if (asset.id.empty()) {
		asset.id = std::move(idStr);
	}
	return makeAsset<T>(asset, store.m_onModified[id]);
}
template <typename T>
template <typename Data>
std::optional<Asset<T>> TAssetMap<T>::load(AssetStore const& store, Resources& res, io::Path const& id, Data&& data) {
	auto idStr = id.generic_string();
	AssetLoader<T> loader;
	// Store calls this without any locks, so obtain unique lock while (potentially) modifying m_storage
	auto lock = store.m_mrsw.lock<std::unique_lock>();
	TAsset<T>& asset = m_storage[idStr];
	AssetStore::OnModified& onModified = store.m_onModified[idStr];
	// Unique lock not needed anymore, and loader.load() may invoke store.find() etc which would need shared locks
	lock.unlock();
	asset.loadInfo = AssetLoadInfo<T>(store, res, onModified, std::forward<Data>(data), id);
	asset.t = loader.load(*asset.loadInfo);
	if (asset.t) {
		conf::g_log.log(dl::level::info, 1, "== [Asset] [{}] loaded", idStr);
		if (asset.id.empty()) {
			asset.id = std::move(idStr);
		}
		return makeAsset<T>(asset, onModified);
	}
	conf::g_log.log(dl::level::warning, 0, "[Asset] Failed to load [{}]!", idStr);
	return std::nullopt;
}
template <typename T>
bool TAssetMap<T>::reloadAsset(AssetStore const& store, TAsset<T>& out_asset) const {
	auto lock = store.m_mutex.lock<std::unique_lock>();
	out_asset.loadInfo->forceDirty(false);
	AssetLoader<T> loader;
	if (loader.reload(*out_asset.t, *out_asset.loadInfo)) {
		AssetStore::OnModified& onModified = store.m_onModified[out_asset.id];
		lock.unlock();
		conf::g_log.log(dl::level::info, 1, "== [Asset] [{}] reloaded", out_asset.id);
		onModified();
		return true;
	} else {
		conf::g_log.log(dl::level::warning, 0, "[Asset] Failed to reload [{}]!", out_asset.id);
	}
	return false;
}
template <typename T>
bool TAssetMap<T>::reload(AssetStore const& store, Hash id) {
	if (auto it = m_storage.find(id); it != m_storage.end()) {
		return reloadAsset(store, it->second);
	}
	return false;
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
u64 TAssetMap<T>::update(AssetStore const& store) {
	u64 ret = 0;
	for (auto& [_, asset] : m_storage) {
		if (asset.t && asset.loadInfo && asset.loadInfo->modified()) {
			reloadAsset(store, asset);
			++ret;
		}
	}
	return ret;
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
	auto lock = m_mrsw.lock<std::unique_lock>();
	return m_assets.get<T>().add(*this, id, std::forward<T>(t));
}
template <typename T, typename Data>
std::optional<Asset<T>> AssetStore::load(io::Path const& id, Data&& data) {
	// AssetLoader may invoke find() etc which would need shared locks, so
	// cannot use unique_lock for the entire function call here
	auto lock = m_mrsw.lock<std::shared_lock>();
	auto& map = m_assets.get<T>();
	lock.unlock();
	// AssetMap optimally unique_locks mutex (performs load outside lock)
	return map.load(*this, m_resources, id, std::forward<Data>(data));
}
template <typename T>
std::optional<Asset<T>> AssetStore::find(Hash id) {
	auto lock = m_mrsw.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		auto& store = m_assets.get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) {
			return detail::makeAsset<T>(it->second, m_onModified[id]);
		}
	}
	return std::nullopt;
}
template <typename T>
std::optional<Asset<T const>> AssetStore::find(Hash id) const {
	auto lock = m_mrsw.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		auto& store = m_assets.get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) {
			return detail::makeAsset<T const>(it->second, m_onModified[id]);
		}
	}
	return std::nullopt;
}
template <typename T>
Asset<T> AssetStore::get(Hash id) {
	auto lock = m_mrsw.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		auto& store = m_assets.get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) {
			return detail::makeAsset<T>(it->second, m_onModified[id]);
		}
	}
	ENSURE(false, "Asset not found!");
	throw std::runtime_error("Asset not present");
}
template <typename T>
Asset<T const> AssetStore::get(Hash id) const {
	auto lock = m_mrsw.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		auto& store = m_assets.get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) {
			return detail::makeAsset<T>(it->second, m_onModified[id]);
		}
	}
	ENSURE(false, "Asset not found!");
	throw std::runtime_error("Asset not present");
}
template <typename T>
bool AssetStore::contains(Hash id) const noexcept {
	auto lock = m_mrsw.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		return detail::mapContains(m_assets.get<T>().m_storage, id);
	}
	return false;
}
template <typename T>
bool AssetStore::reload(Hash id) {
	auto lock = m_mrsw.lock<std::shared_lock>();
	if (m_assets.contains<T>()) {
		return m_assets.get<T>().reload(id);
	}
	return false;
}
template <typename T>
bool AssetStore::unload(Hash id) {
	auto lock = m_mrsw.lock<std::unique_lock>();
	if (m_assets.contains<T>()) {
		return m_assets.get<T>().unload(id);
	}
	return false;
}
template <template <typename...> typename L>
L<std::mutex> AssetStore::lock() const {
	return m_mutex.lock<L>();
}
inline Resources& AssetStore::resources() {
	return m_resources;
}

template <typename T>
Asset<T>::Asset(type& t, OnModified& onMod, Hash id) : m_id(id), m_t(t), m_onModified(onMod) {
}
template <typename T>
Asset<T const>::Asset(type const& t, OnModified& onMod, Hash id) : m_id(id), m_t(t), m_onModified(onMod) {
}
template <typename T>
typename Asset<T>::type& Asset<T>::get() const {
	return m_t;
}
template <typename T>
typename Asset<T>::type& Asset<T>::operator*() {
	return get();
}
template <typename T>
typename Asset<T>::type& Asset<T>::operator->() {
	return &get();
}
template <typename T>
typename Asset<T>::OnModified::Tk Asset<T>::onModified(OnModified::Callback const& callback) {
	return m_onModified.get().subscribe(callback);
}
template <typename T>
typename Asset<T const>::type const& Asset<T const>::get() const {
	return m_t;
}
template <typename T>
typename Asset<T const>::type const& Asset<T const>::operator*() {
	return get();
}
template <typename T>
typename Asset<T const>::type const& Asset<T const>::operator->() {
	return &get();
}
template <typename T>
typename Asset<T const>::OnModified::Tk Asset<T const>::onModified(OnModified::Callback const& callback) {
	return m_onModified.get().subscribe(callback);
}
} // namespace le
