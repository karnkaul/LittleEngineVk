#pragma once
#include <ktl/async/kmutex.hpp>
#include <levk/core/hash.hpp>
#include <levk/core/log_channel.hpp>
#include <levk/core/utils/string.hpp>
#include <levk/engine/assets/asset_loader.hpp>
#include <levk/engine/assets/resources.hpp>
#include <atomic>
#include <memory>

namespace le::foo {
class AssetStore {
  public:
	using Sign = std::size_t;
	using OnModified = ktl::delegate<>;
	struct Index;

	template <typename T>
	static Sign sign();
	template <typename... T>
	static Span<Sign const> signs();

	template <typename T>
	Opt<T> add(std::string uri, T t);
	template <typename T>
	Opt<T> add(std::string uri, AssetStorage<T>&& t);
	template <typename T>
	Opt<T> load(std::string uri, AssetLoadData<T> data);

	bool exists(Hash uri) const;
	template <typename T>
	bool exists(Hash uri) const;
	template <typename T>
	Opt<T> find(Hash uri) const;
	bool unload(Hash uri);
	template <typename T>
	bool unload(Hash uri);

	template <typename T>
		requires(detail::reloadable_asset_v<T>)
	bool reload(Hash uri);
	void checkModified();
	OnModified::signal onModified(Hash uri);
	std::size_t size() const;
	void clear();

	Resources& resources() noexcept { return m_resources; }
	Resources const& resources() const noexcept { return m_resources; }

	template <typename... Ts>
	Index index(std::string_view filter = {}) const;
	Index index(Span<Sign const> signs, std::string_view filter) const;

  private:
	struct Base;
	template <typename T>
	struct TAsset;
	using TAssets = std::unordered_map<Hash, std::unique_ptr<Base>>;
	using DoReload = ktl::kfunction<bool(Base&)>;
	using OnModMap = std::unordered_map<Hash, OnModified>;

	template <typename T>
	Opt<T> add(std::unique_ptr<TAsset<T>>&& tasset);
	template <typename T>
	DoReload doReload();
	template <typename T>
		requires(detail::reloadable_asset_v<T>)
	bool reloadAsset(TAsset<T>& out_asset);
	void modified(Hash uri);

	inline static std::atomic<std::size_t> s_nextSign{};

	ktl::strict_tmutex<TAssets> m_assets;
	ktl::strict_tmutex<OnModMap> m_onModified;
	Resources m_resources;
};

struct AssetStore::Index {
	struct Type {
		std::string_view name;
		Sign sign;
	};
	struct Map {
		Type type;
		std::vector<std::string_view> uris;
	};

	std::vector<Map> maps;
};

// impl

struct AssetStore::Base {
	std::string uri;
	std::string_view typeName;
	DoReload doReload;
	Sign sign{};

	Base(std::string uri, Sign sign, std::string_view typeName, DoReload&& doReload) noexcept
		: uri(std::move(uri)), typeName(typeName), doReload(std::move(doReload)), sign(sign) {}
	virtual ~Base() = default;
	virtual bool dirty() const = 0;
};

template <typename T>
struct AssetStore::TAsset : Base {
	std::optional<AssetLoadInfo<T>> info;
	AssetStorage<T> asset;

	static std::string_view tname() { return utils::removeNamespaces(utils::tName<T>()); }

	TAsset(std::string&& uri, AssetStorage<T>&& asset) noexcept : Base(std::move(uri), AssetStore::sign<T>(), tname(), {}), asset(std::move(asset)) {}
	TAsset(std::string&& uri, AssetLoadInfo<T>&& info, DoReload&& doReload = {}) noexcept
		: Base(std::move(uri), AssetStore::sign<T>(), tname(), std::move(doReload)), info(std::move(info)) {}
	bool dirty() const override { return info && info->modified(); }
};

template <typename T>
AssetStore::Sign AssetStore::sign() {
	static Sign const ret = ++s_nextSign;
	return ret;
}

template <typename... T>
Span<AssetStore::Sign const> AssetStore::signs() {
	if constexpr (sizeof...(T) > 0) {
		thread_local Sign const ret[] = {sign<T>()...};
		return ret;
	} else {
		return {};
	}
}

template <typename T>
Opt<T> AssetStore::add(std::string uri, T t) {
	return add(std::move(uri), makeAssetStorage<T>(std::move(t)));
}

template <typename T>
Opt<T> AssetStore::add(std::string uri, AssetStorage<T>&& asset) {
	if (!asset) { return {}; }
	logI(LC_EndUser, "[Asset] [{}] added", uri);
	return add(std::make_unique<TAsset<T>>(std::move(uri), std::move(asset)));
}

template <typename T>
Opt<T> AssetStore::load(std::string uri, AssetLoadData<T> data) {
	auto info = AssetLoadInfo<T>(this, &m_resources, std::move(data), uri);
	auto tasset = std::make_unique<TAsset<T>>(std::move(uri), std::move(info), doReload<T>());
	if ((tasset->asset = AssetLoader<T>{}.load(*tasset->info)); tasset->asset) {
		logI(LC_EndUser, "[Asset] [{}] loaded", tasset->uri);
		return add(std::move(tasset));
	}
	logW(LC_EndUser, "[Asset] Failed to load [{}]!", tasset->uri);
	return {};
}

template <typename T>
bool AssetStore::exists(Hash uri) const {
	ktl::klock lock(m_assets);
	auto it = lock->find(uri);
	return it != lock->end() && dynamic_cast<TAsset<T>*>(it->second.get()) != nullptr;
}

template <typename T>
	requires(detail::reloadable_asset_v<T>)
bool AssetStore::reload(Hash uri) {
	TAsset<T>* tasset{};
	{
		ktl::klock lock(m_assets);
		if (auto it = lock->find(uri); it != lock->end()) {
			if (it->second->sign == sign<T>()) { tasset = &toTAsset<T>(*it->second); }
		}
	}
	if (tasset) { return reloadAsset(*tasset); }
	return false;
}

template <typename T>
Opt<T> AssetStore::find(Hash uri) const {
	ktl::klock lock(m_assets);
	auto it = lock->find(uri);
	if (it != lock->end()) {
		if (auto tasset = dynamic_cast<TAsset<T>*>(it->second.get())) {
			auto& ret = tasset->asset;
			EXPECT(ret.has_value());
			return &*ret;
		}
	}
	return {};
}

template <typename T>
bool AssetStore::unload(Hash uri) {
	ktl::klock lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end() && it->second->sign == sign<T>()) {
		lock->erase(it);
		return true;
	}
	return false;
}

template <typename... Ts>
AssetStore::Index AssetStore::index(std::string_view filter) const {
	return index(sign<Ts...>(), filter);
}

template <typename T>
Opt<T> AssetStore::add(std::unique_ptr<TAsset<T>>&& tasset) {
	EXPECT(tasset);
	Hash const hash = tasset->uri;
	ktl::klock lock(m_assets);
	auto [it, b] = lock->insert_or_assign(hash, std::move(tasset));
	if (!b) { logW(LC_EndUser, "[Asset] [{} ({})] overwritten!", it->second->uri, it->first.hash); }
	EXPECT(it->second->sign == sign<T>());
	return &*static_cast<TAsset<T>*>(it->second.get())->asset;
}

template <typename T>
	requires(detail::reloadable_asset_v<T>)
bool AssetStore::reloadAsset(TAsset<T>& out_asset) {
	if (AssetLoader<T>{}.reload(*out_asset.asset, *out_asset.info)) {
		logI(LC_LibUser, "== [Asset] [{}] reloaded", out_asset.uri);
		modified(out_asset.uri);
		return true;
	}
	return false;
}

template <typename T>
auto AssetStore::doReload() -> DoReload {
	if constexpr (detail::reloadable_asset_v<T>) {
		return [this](Base& base) { return reloadAsset(static_cast<TAsset<T>&>(base)); };
	} else {
		return {};
	}
}
} // namespace le::foo
