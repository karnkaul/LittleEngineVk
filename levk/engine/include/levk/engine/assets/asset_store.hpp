#pragma once
#include <ktl/async/kmutex.hpp>
#include <ktl/delegate.hpp>
#include <levk/core/log_channel.hpp>
#include <levk/core/std_types.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/core/utils/string.hpp>
#include <levk/core/utils/type_guid.hpp>
#include <levk/engine/assets/asset_loader.hpp>

namespace le {
class AssetStore : public NoCopy {
  public:
	using OnModified = ktl::delegate<>;
	using Sign = utils::TypeGUID;

	struct Index;

	template <typename T>
	Opt<T> add(std::string uri, T t);
	template <typename T>
	Opt<T> add(std::string uri, AssetStorage<T>&& t);
	template <typename T>
	Opt<T> load(std::string uri, AssetLoadData<T> data);
	template <typename T>
	Opt<T> find(Hash uri) const;
	bool exists(Hash uri) const;
	template <typename T>
	bool exists(Hash uri) const;
	bool unload(Hash uri);
	template <typename T>
	bool unload(Hash uri);
	template <typename T>
		requires(detail::reloadable_asset_v<T>)
	bool reload(Hash uri);
	OnModified::signal onModified(Hash uri);

	void checkModified();
	std::size_t size() const;
	void clear();

	Resources& resources() noexcept { return m_resources; }
	Resources const& resources() const noexcept { return m_resources; }

	template <typename T>
	static Sign sign();
	template <typename... T>
	static Span<Sign const> signs();

	template <typename... Ts>
	Index index(std::string_view filter = {}) const;
	Index index(Span<Sign const> signs, std::string_view filter) const;

  private:
	struct Base;
	using DoReload = ktl::kfunction<bool(Base&)>;
	using TAssets = std::unordered_map<Hash, std::unique_ptr<Base>>;
	using OnModMap = std::unordered_map<Hash, OnModified>;
	template <typename T>
	struct TAsset;

	template <typename T>
	Opt<T> add(std::unique_ptr<TAsset<T>>&& tasset);
	void modified(Hash uri);

	template <typename T>
		requires(detail::reloadable_asset_v<T>)
	bool reloadAsset(TAsset<T>& out_asset);

	template <typename T>
	static TAsset<T>& toTAsset(Base& base) noexcept;

	template <typename T>
	static Opt<T> toT(Base& base);

	template <typename T>
	DoReload doReload();

	Resources m_resources;
	ktl::strict_tmutex<TAssets> m_assets;
	ktl::strict_tmutex<OnModMap> m_onModified;
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
	Sign sign;
	Base(std::string uri, std::string_view typeName, Sign sign, DoReload&& doReload) noexcept
		: uri(std::move(uri)), typeName(typeName), doReload(std::move(doReload)), sign(sign) {}
	virtual ~Base() = default;
	virtual bool dirty() const = 0;
};

template <typename T>
struct AssetStore::TAsset : Base {
	std::optional<AssetLoadInfo<T>> info;
	AssetStorage<T> t;

	static std::string_view tName() { return utils::removeNamespaces(utils::tName<T>()); }

	TAsset(std::string&& uri, AssetStorage<T>&& t) noexcept : Base(std::move(uri), tName(), AssetStore::sign<T>(), {}), t(std::move(t)) {}
	TAsset(std::string&& uri, AssetLoadInfo<T>&& info, DoReload&& doReload) noexcept
		: Base(std::move(uri), tName(), AssetStore::sign<T>(), std::move(doReload)), info(std::move(info)) {}
	bool dirty() const override { return info && info->modified(); }
};

template <typename T>
Opt<T> AssetStore::add(std::string uri, T t) {
	return add(std::move(uri), makeAssetStorage<T>(std::move(t)));
}

template <typename T>
Opt<T> AssetStore::add(std::string uri, AssetStorage<T>&& t) {
	return add(std::make_unique<TAsset<T>>(std::move(uri), std::move(t)));
}

template <typename T>
Opt<T> AssetStore::load(std::string uri, AssetLoadData<T> data) {
	auto info = AssetLoadInfo<T>(this, &m_resources, std::move(data), uri);
	auto tasset = std::make_unique<TAsset<T>>(std::move(uri), std::move(info), doReload<T>());
	if ((tasset->t = AssetLoader<T>{}.load(*tasset->info)); tasset->t) { return add(std::move(tasset)); }
	logW(LC_EndUser, "[Asset] Failed to load [{}]!", tasset->uri);
	return {};
}

template <typename T>
Opt<T> AssetStore::find(Hash uri) const {
	ktl::klock lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		EXPECT(it->second);
		if (it->second->sign == sign<T>()) { return toT<T>(*it->second); }
	}
	return {};
}

template <typename T>
bool AssetStore::exists(Hash uri) const {
	return find<T>(uri) != nullptr;
}

template <typename T>
	requires(detail::reloadable_asset_v<T>)
bool AssetStore::reload(Hash uri) {
	TAsset<T>* tasset{};
	{
		ktl::klock lock(m_assets);
		if (auto it = lock->find(uri); it != lock->end()) {
			EXPECT(it->second);
			if (it->second->sign == sign<T>()) { tasset = &toTAsset<T>(*it->second); }
		}
	}
	if (tasset) { return reloadAsset(*tasset); }
	return false;
}

template <typename T>
bool AssetStore::unload(Hash uri) {
	ktl::klock lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		EXPECT(it->second);
		if (it->second->sign == sign<T>()) {
			lock->erase(it);
			return true;
		}
	}
	return false;
}

template <typename T>
Opt<T> AssetStore::add(std::unique_ptr<TAsset<T>>&& tasset) {
	if (!tasset) { return {}; }
	Hash const key = tasset->uri;
	ktl::klock lock(m_assets);
	auto const [it, b] = lock->insert_or_assign(key, std::move(tasset));
	if (!b) { logW(LC_EndUser, "[Asset] Overwriting [{}]!", it->second->uri); }
	logI(LC_EndUser, "== [Asset] [{}] added", it->second->uri);
	return toT<T>(*it->second);
}

template <typename T>
	requires(detail::reloadable_asset_v<T>)
bool AssetStore::reloadAsset(TAsset<T>& out_asset) {
	if (AssetLoader<T>{}.reload(*out_asset.t, *out_asset.info)) {
		logI(LC_LibUser, "== [Asset] [{}] reloaded", out_asset.uri);
		modified(out_asset.uri);
		return true;
	}
	return false;
}

template <typename T>
auto AssetStore::toTAsset(Base& base) noexcept -> TAsset<T>& {
	return static_cast<TAsset<T>&>(base);
}

template <typename T>
Opt<T> AssetStore::toT(Base& base) {
	auto& t = toTAsset<T>(base).t;
	return t ? &*t : nullptr;
}

template <typename T>
auto AssetStore::doReload() -> DoReload {
	if constexpr (detail::reloadable_asset_v<T>) {
		return [this](Base& base) { return reloadAsset(toTAsset<T>(base)); };
	} else {
		return {};
	}
}

template <typename T>
AssetStore::Sign AssetStore::sign() {
	return Sign::make<T>();
}

template <typename... T>
Span<AssetStore::Sign const> AssetStore::signs() {
	if constexpr (sizeof...(T) > 0) {
		static Sign const ret[] = {sign<T>()...};
		return ret;
	} else {
		return {};
	}
}

template <typename... Ts>
AssetStore::Index AssetStore::index(std::string_view filter) const {
	return index(sign<Ts...>(), filter);
}
} // namespace le
