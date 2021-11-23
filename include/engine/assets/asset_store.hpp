#pragma once
#include <core/log.hpp>
#include <core/utils/algo.hpp>
#include <engine/assets/asset.hpp>
#include <engine/assets/asset_loader.hpp>
#include <engine/utils/logger.hpp>
#include <ktl/shared_tmutex.hpp>
#include <memory>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>

namespace le {
namespace detail {
class AssetMap;
using AssetMapPtr = std::unique_ptr<AssetMap>;
template <typename T>
struct TAsset;
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

class AssetStore : public NoCopy {
  public:
	using OnModified = ktl::delegate<>;

	template <typename T>
	Asset<T> add(io::Path const& uri, T t);
	template <typename T>
	Asset<T> add(io::Path const& uri, std::unique_ptr<T> t);
	template <typename T>
	Asset<T> load(io::Path const& uri, AssetLoadData<T> data);
	template <typename T>
	Asset<T> find(Hash id) const;
	template <typename T>
	bool exists(Hash id) const noexcept;
	template <typename T>
	bool unload(Hash id);
	template <typename T>
	bool reload(Hash id);

	void update();
	void clear();

	template <template <typename...> typename L = std::scoped_lock>
	L<std::mutex> reloadLock() const;

	Resources& resources() noexcept { return m_resources; }
	Resources const& resources() const noexcept { return m_resources; }

  private:
	template <typename T>
	bool reloadAsset(detail::TAsset<T>& out_asset) const;

	Resources m_resources;
	ktl::shared_strict_tmutex<detail::TAssets> m_assets;
	mutable ktl::strict_tmutex<std::unordered_map<Hash, OnModified>> m_onModified;
	mutable std::mutex m_reloadMutex;

	template <typename T>
	friend class detail::TAssetMap;
};

// impl

namespace detail {
template <typename T>
struct TAsset {
	std::string id;
	std::optional<AssetLoadInfo<T>> loadInfo;
	std::unique_ptr<T> t;
};

class AssetMap {
  public:
	virtual ~AssetMap() = default;
	virtual u64 update(AssetStore const& store) = 0;
};

template <typename T>
class TAssetMap : public AssetMap {
  public:
	using OnModified = AssetStore::OnModified;

	template <typename... Args>
	Asset<T> add(OnModified& onMod, io::Path const& uri, Args&&... args);
	static std::optional<TAsset<T>> load(AssetStore const& store, OnModified& onMod, Resources& res, std::string id, AssetLoadData<T> data);
	Asset<T> insert(TAsset<T>&& asset, AssetStore::OnModified& onMod);
	bool reload(AssetStore const& store, Hash id);
	bool unload(Hash id);
	bool forceDirty(Hash id) const;
	u64 update(AssetStore const& store) override;

	using Storage = std::unordered_map<Hash, TAsset<T>>;
	Storage m_storage;
};

template <typename T>
constexpr Asset<T> wrap(TAsset<T>& u, AssetStore::OnModified& onModified) noexcept {
	return {&*u.t, u.id, &onModified};
}

template <typename T>
template <typename... Args>
Asset<T> TAssetMap<T>::add(AssetStore::OnModified& onMod, io::Path const& uri, Args&&... args) {
	TAsset<T> asset{uri.generic_string(), {}, std::make_unique<T>(std::forward<Args>(args)...)};
	auto const [it, nascent] = m_storage.emplace(uri.generic_string(), std::move(asset));
	TAsset<T>& ret = it->second;
	if (!nascent) { utils::g_log.log(dl::level::warn, 0, "[Asset] Overwriting [{}]!", ret.id); }
	utils::g_log.log(dl::level::info, 1, "== [Asset] [{}] added", ret.id);
	return wrap<T>(ret, onMod);
}
template <typename T>
std::optional<TAsset<T>> TAssetMap<T>::load(AssetStore const& store, AssetStore::OnModified& onMod, Resources& res, std::string id, AssetLoadData<T> data) {
	AssetLoader<T> loader;
	TAsset<T> asset{id, AssetLoadInfo<T>(&store, &res, &onMod, std::move(data), std::move(id)), {}};
	if ((asset.t = loader.load(*asset.loadInfo))) {
		utils::g_log.log(dl::level::info, 1, "== [Asset] [{}] loaded", asset.id);
		return asset;
	}
	utils::g_log.log(dl::level::warn, 0, "[Asset] Failed to load [{}]!", id);
	return std::nullopt;
}
template <typename T>
Asset<T> TAssetMap<T>::insert(TAsset<T>&& asset, AssetStore::OnModified& onMod) {
	Hash const id = asset.id;
	auto const [it, bNew] = m_storage.insert({id, std::move(asset)});
	if (!bNew) { utils::g_log.log(dl::level::warn, 0, "[Asset] Overwriting [{}]!", asset.id); }
	return wrap<T>(it->second, onMod);
}
template <typename T>
bool TAssetMap<T>::reload(AssetStore const& store, Hash id) {
	if constexpr (detail::reloadable_asset_v<T>) {
		if (auto it = m_storage.find(id); it != m_storage.end()) {
			auto& asset = it->second;
			if (store.reloadAsset<T>(asset)) {
				utils::g_log.log(dl::level::info, 1, "== [Asset] [{}] reloaded", asset.id);
				return true;
			} else {
				utils::g_log.log(dl::level::warn, 0, "[Asset] Failed to reload [{}]!", asset.id);
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
				if (store.reloadAsset<T>(asset)) {
					utils::g_log.log(dl::level::info, 1, "== [Asset] [{}] reloaded", asset.id);
					++ret;
				} else {
					utils::g_log.log(dl::level::warn, 0, "[Asset] Failed to reload [{}]!", asset.id);
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
Asset<T> AssetStore::add(io::Path const& uri, T t) {
	ktl::unique_tlock<detail::TAssets> lock(m_assets);
	return lock.get().get<T>().add(ktl::tlock(m_onModified).get()[uri], uri, std::move(t));
}
template <typename T>
Asset<T> AssetStore::add(io::Path const& uri, std::unique_ptr<T> t) {
	ktl::unique_tlock<detail::TAssets> lock(m_assets);
	return lock.get().get<T>().add(ktl::tlock(m_onModified).get()[uri], uri, std::move(t));
}
template <typename T>
Asset<T> AssetStore::load(io::Path const& uri, AssetLoadData<T> data) {
	auto idStr = uri.generic_string();
	auto& onMod = ktl::tlock(m_onModified).get()[idStr];
	// AssetLoader may invoke find() etc which would need shared locks
	if (auto asset = detail::TAssetMap<T>::load(*this, onMod, m_resources, std::move(idStr), std::move(data))) {
		ktl::unique_tlock<detail::TAssets> lock(m_assets);
		return lock.get().get<T>().insert(std::move(*asset), onMod);
	}
	return {};
}
template <typename T>
Asset<T> AssetStore::find(Hash id) const {
	ktl::shared_tlock<detail::TAssets const> lock(m_assets);
	if (lock.get().contains<T>()) {
		auto& store = lock.get().get<T>().m_storage;
		if (auto it = store.find(id); it != store.end() && it->second.t) { return detail::wrap<T>(it->second, ktl::tlock(m_onModified).get()[id]); }
	}
	return {};
}
template <typename T>
bool AssetStore::exists(Hash id) const noexcept {
	ktl::shared_tlock<detail::TAssets const> lock(m_assets);
	if (lock.get().contains<T>()) { return utils::contains(lock.get().get<T>().m_storage, id); }
	return false;
}
template <typename T>
bool AssetStore::reload(Hash id) {
	if constexpr (detail::reloadable_asset_v<T>) {
		ktl::shared_tlock<detail::TAssets> lock(m_assets);
		if (lock.get().contains<T>()) { return lock.get().get<T>().reload(*this, id); }
	}
	return false;
}
template <typename T>
bool AssetStore::unload(Hash id) {
	ktl::unique_tlock<detail::TAssets> lock(m_assets);
	if (lock.get().contains<T>()) { return lock.get().get<T>().unload(id); }
	return false;
}
template <template <typename...> typename L>
L<std::mutex> AssetStore::reloadLock() const {
	return L<std::mutex>(m_reloadMutex);
}
template <typename T>
bool AssetStore::reloadAsset(detail::TAsset<T>& out_asset) const {
	if constexpr (detail::reloadable_asset_v<T>) {
		auto lock = reloadLock();
		out_asset.loadInfo->forceDirty(false);
		AssetLoader<T> loader;
		if (loader.reload(*out_asset.t, *out_asset.loadInfo)) {
			ktl::tlock(m_onModified).get()[out_asset.loadInfo->m_id]();
			return true;
		}
	}
	return false;
}
} // namespace le
