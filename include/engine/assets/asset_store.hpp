#pragma once
#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>
#include <core/log.hpp>
#include <core/utils/algo.hpp>
#include <engine/assets/asset_loader.hpp>
#include <engine/utils/logger.hpp>
#include <kt/tmutex/shared_tmutex.hpp>

namespace le {
namespace detail {
class AssetMap;
using AssetMapPtr = std::unique_ptr<AssetMap>;
template <typename T>
class TAssetMap;

struct TAssets final {
	template <typename Value>
	TAssetMap<Value>& get() const;
	template <typename Value>
	bool contains() const noexcept;
	template <typename Value>
	std::size_t hash() const;

	mutable std::unordered_map<std::size_t, AssetMapPtr> storeMap;
};
} // namespace detail

template <typename T>
class Asset;

template <typename T>
using OptAsset = std::optional<Asset<T>>;

class AssetStore : public NoCopy {
  public:
	using OnModified = Delegate<>;

	AssetStore() = default;
	~AssetStore();

	template <typename T>
	Asset<T> add(io::Path const& id, T t);
	template <typename T>
	OptAsset<T> load(io::Path const& id, AssetLoadData<T> data);
	template <typename T>
	OptAsset<T> find(Hash id) const;
	template <typename T>
	Asset<T> get(Hash id) const;
	template <typename T>
	bool exists(Hash id) const noexcept;
	template <typename T>
	bool unload(Hash id);
	template <typename T>
	bool reload(Hash id);
	template <typename T>
	bool forceDirty(Hash id) const;

	void update();
	void clear();

	template <template <typename...> typename L = std::scoped_lock>
	L<std::mutex> reloadLock() const;

	Resources& resources();

  private:
	template <typename T>
	bool reloadAsset(T& out_asset, AssetLoadInfo<T> const& info) const;

	Resources m_resources;
	kt::shared_strict_tmutex<detail::TAssets> m_assets;
	mutable kt::strict_tmutex<std::unordered_map<Hash, OnModified>> m_onModified;
	mutable std::mutex m_reloadMutex;

	template <typename T>
	friend class detail::TAssetMap;
};

template <typename T>
class Asset {
  public:
	using type = T;
	using OnModified = AssetStore::OnModified;

	Asset(not_null<type*> t, not_null<OnModified*> onMod, std::string_view id);

	type& get() const;
	type& operator*() const;
	type* operator->() const;
	OnModified::Tk onModified(OnModified::Callback const& callback);

	std::string_view m_id;

  private:
	not_null<T*> m_t;
	not_null<OnModified*> m_onModified;
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
	Asset<T> add(AssetStore::OnModified& onMod, io::Path const& id, T t);
	template <typename Data>
	static std::optional<TAsset<T>> load(AssetStore const& store, AssetStore::OnModified& onMod, Resources& res, std::string id, Data&& data);
	Asset<T> insert(TAsset<T>&& asset, AssetStore::OnModified& onMod);
	bool reload(AssetStore const& store, Hash id);
	bool unload(Hash id);
	bool forceDirty(Hash id) const;
	u64 update(AssetStore const& store) override;

	using Storage = std::unordered_map<Hash, TAsset<T>>;
	Storage m_storage;
};

template <typename T, typename U>
constexpr Asset<T> makeAsset(U&& wrap, AssetStore::OnModified& onModified) noexcept {
	return {&*wrap.t, &onModified, wrap.id};
}

template <typename T>
Asset<T> TAssetMap<T>::add(AssetStore::OnModified& onMod, io::Path const& id, T t) {
	auto idStr = id.generic_string();
	auto const [it, nascent] = m_storage.emplace(idStr, TAsset<T>{});
	if (!nascent) { utils::g_log.log(dl::level::warning, 0, "[Asset] Overwriting [{}]!", idStr); }
	TAsset<T>& asset = it->second;
	asset.t.emplace(std::move(t));
	asset.loadInfo.reset();
	asset.id = std::move(idStr);
	utils::g_log.log(dl::level::info, 1, "== [Asset] [{}] added", asset.id);
	return makeAsset<T>(asset, onMod);
}
template <typename T>
template <typename Data>
std::optional<TAsset<T>> TAssetMap<T>::load(AssetStore const& store, AssetStore::OnModified& onMod, Resources& res, std::string id, Data&& data) {
	AssetLoader<T> loader;
	TAsset<T> asset;
	asset.loadInfo = AssetLoadInfo<T>(&store, &res, &onMod, std::forward<Data>(data), id);
	asset.t = loader.load(*asset.loadInfo);
	if (asset.t) {
		utils::g_log.log(dl::level::info, 1, "== [Asset] [{}] loaded", id);
		asset.id = std::move(id);
		return asset;
	}
	utils::g_log.log(dl::level::warning, 0, "[Asset] Failed to load [{}]!", id);
	return std::nullopt;
}
template <typename T>
Asset<T> TAssetMap<T>::insert(TAsset<T>&& asset, AssetStore::OnModified& onMod) {
	Hash const id = asset.id;
	auto const [it, bNew] = m_storage.insert({id, std::move(asset)});
	if (!bNew) { utils::g_log.log(dl::level::warning, 0, "[Asset] Overwriting [{}]!", asset.id); }
	return makeAsset<T>(it->second, onMod);
}
template <typename T>
bool TAssetMap<T>::reload(AssetStore const& store, Hash id) {
	if constexpr (detail::reloadable_asset_v<T>) {
		if (auto it = m_storage.find(id); it != m_storage.end()) {
			auto& asset = it->second;
			if (store.reloadAsset<T>(*asset.t, *asset.loadInfo)) {
				utils::g_log.log(dl::level::info, 1, "== [Asset] [{}] reloaded", asset.id);
				return true;
			} else {
				utils::g_log.log(dl::level::warning, 0, "[Asset] Failed to reload [{}]!", asset.id);
			}
		}
	}
	return false;
}
template <typename T>
bool TAssetMap<T>::unload(Hash id) {
	if (auto it = m_storage.find(id); it != m_storage.end()) {
		utils::g_log.log(dl::level::info, 1, "-- [Asset] [{}] unloaded", it->second.id);
		m_storage.erase(it);
		return true;
	}
	return false;
}
template <typename T>
bool TAssetMap<T>::forceDirty(Hash id) const {
	if (auto it = m_storage.find(id); it != m_storage.end() && it->second.loadInfo) {
		it->second.loadInfo->forceDirty(true);
		return true;
	}
	return false;
}
template <typename T>
u64 TAssetMap<T>::update(AssetStore const& store) {
	u64 ret = 0;
	if constexpr (detail::reloadable_asset_v<T>) {
		for (auto& [_, asset] : m_storage) {
			if (asset.t && asset.loadInfo && asset.loadInfo->modified()) {
				if (store.reloadAsset<T>(*asset.t, *asset.loadInfo)) {
					utils::g_log.log(dl::level::info, 1, "== [Asset] [{}] reloaded", asset.id);
					++ret;
				} else {
					utils::g_log.log(dl::level::warning, 0, "[Asset] Failed to reload [{}]!", asset.id);
				}
			}
		}
	}
	return ret;
}

template <typename Value>
TAssetMap<Value>& TAssets::get() const {
	auto search = storeMap.find(hash<Value>());
	if (search == storeMap.end()) {
		auto [it, _] = storeMap.emplace(hash<Value>(), std::make_unique<TAssetMap<Value>>());
		search = it;
	}
	return static_cast<TAssetMap<Value>&>(*search->second);
}
template <typename Value>
bool TAssets::contains() const noexcept {
	return utils::contains(storeMap, hash<Value>());
}
template <typename Value>
std::size_t TAssets::hash() const {
	return typeid(std::decay_t<Value>).hash_code();
}
} // namespace detail

template <typename T>
Asset<T> AssetStore::add(io::Path const& id, T t) {
	kt::unique_tlock<detail::TAssets> lock(m_assets);
	return lock.get().get<T>().add(kt::tlock(m_onModified).get()[id], id, std::move(t));
}
template <typename T>
OptAsset<T> AssetStore::load(io::Path const& id, AssetLoadData<T> data) {
	auto idStr = id.generic_string();
	auto& onMod = kt::tlock(m_onModified).get()[idStr];
	// AssetLoader may invoke find() etc which would need shared locks
	if (auto asset = detail::TAssetMap<T>::load(*this, onMod, m_resources, std::move(idStr), std::move(data))) {
		kt::unique_tlock<detail::TAssets> lock(m_assets);
		return lock.get().get<T>().insert(std::move(*asset), onMod);
	}
	return std::nullopt;
}
template <typename T>
OptAsset<T> AssetStore::find(Hash id) const {
	kt::shared_tlock<detail::TAssets const> lock(m_assets);
	if (lock.get().contains<T>()) {
		auto& store = lock.get().get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) { return detail::makeAsset<T>(it->second, kt::tlock(m_onModified).get()[id]); }
	}
	return std::nullopt;
}
template <typename T>
Asset<T> AssetStore::get(Hash id) const {
	kt::shared_tlock<detail::TAssets const> lock(m_assets);
	if (lock.get().contains<T>()) {
		auto& store = lock.get().get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) { return detail::makeAsset<T>(it->second, kt::tlock(m_onModified).get()[id]); }
	}
	ensure(false, "Asset not found!");
	throw std::runtime_error("Asset not present");
}
template <typename T>
bool AssetStore::exists(Hash id) const noexcept {
	kt::shared_tlock<detail::TAssets const> lock(m_assets);
	if (lock.get().contains<T>()) { return utils::contains(lock.get().get<T>().m_storage, id); }
	return false;
}
template <typename T>
bool AssetStore::reload(Hash id) {
	if constexpr (detail::reloadable_asset_v<T>) {
		kt::shared_tlock<detail::TAssets> lock(m_assets);
		if (lock.get().contains<T>()) { return lock.get().get<T>().reload(*this, id); }
	}
	return false;
}
template <typename T>
bool AssetStore::forceDirty(Hash id) const {
	kt::shared_tlock<detail::TAssets const> lock(m_assets);
	if (lock.get().contains<T>()) { return lock.get().get<T>().forceDirty(id); }
	return false;
}
template <typename T>
bool AssetStore::unload(Hash id) {
	kt::unique_tlock<detail::TAssets> lock(m_assets);
	if (lock.get().contains<T>()) { return lock.get().get<T>().unload(id); }
	return false;
}
template <template <typename...> typename L>
L<std::mutex> AssetStore::reloadLock() const {
	return L<std::mutex>(m_reloadMutex);
}
inline Resources& AssetStore::resources() { return m_resources; }
template <typename T>
bool AssetStore::reloadAsset(T& out_asset, AssetLoadInfo<T> const& info) const {
	if constexpr (detail::reloadable_asset_v<T>) {
		auto lock = reloadLock();
		info.forceDirty(false);
		AssetLoader<T> loader;
		if (loader.reload(out_asset, info)) {
			kt::tlock(m_onModified).get()[info.m_id]();
			return true;
		}
	}
	return false;
}

template <typename T>
Asset<T>::Asset(not_null<type*> t, not_null<OnModified*> onMod, std::string_view id) : m_id(id), m_t(t), m_onModified(onMod) {}
template <typename T>
typename Asset<T>::type& Asset<T>::get() const {
	return *m_t;
}
template <typename T>
typename Asset<T>::type& Asset<T>::operator*() const {
	return get();
}
template <typename T>
typename Asset<T>::type* Asset<T>::operator->() const {
	return &get();
}
template <typename T>
typename Asset<T>::OnModified::Tk Asset<T>::onModified(OnModified::Callback const& callback) {
	return m_onModified->subscribe(callback);
}

template <typename T>
template <typename U>
void AssetLoadInfo<T>::reloadDepend(Asset<U>& out_asset) const {
	m_tokens.push_back(out_asset.onModified([s = m_store, id = m_id]() { s->template forceDirty<T>(id); }));
}
} // namespace le
