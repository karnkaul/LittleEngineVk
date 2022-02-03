#pragma once
#include <dens/detail/sign.hpp>
#include <ktl/async/kmutex.hpp>
#include <ktl/delegate.hpp>
#include <levk/core/log_channel.hpp>
#include <levk/core/std_types.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/core/utils/string.hpp>
#include <levk/engine/assets/asset_loader.hpp>

namespace le {
class AssetStore : public NoCopy {
  public:
	using OnModified = ktl::delegate<>;
	using Sign = dens::detail::sign_t;

	struct Index;

	template <typename T>
	Opt<T> add(std::string uri, T t);
	template <typename T>
	Opt<T> add(std::string uri, std::unique_ptr<T>&& t);
	template <typename T>
	Opt<T> load(std::string uri, AssetLoadData<T> data);
	template <typename T>
	Opt<T> find(Hash uri) const;
	bool exists(Hash uri) const noexcept;
	template <typename T>
	bool exists(Hash uri) const noexcept;
	bool unload(Hash uri);
	template <typename T>
	bool unload(Hash uri);
	template <typename T>
		requires(detail::reloadable_asset_v<T>)
	bool reload(Hash uri);
	OnModified::signal onModified(Hash uri);

	void update();
	void clear();

	template <template <typename...> typename L = std::scoped_lock>
	L<std::mutex> reloadLock() const;

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
	using DoReload = ktl::kfunction<bool(Base*)>;
	using TAssets = std::unordered_map<Hash, std::unique_ptr<Base>>;
	template <typename T>
	struct TAsset;

	template <typename T>
	Opt<T> add(std::unique_ptr<TAsset<T>>&& tasset);

	template <typename T>
		requires(detail::reloadable_asset_v<T>)
	bool reloadAsset(TAsset<T>& out_asset) const;

	template <typename T>
	static Opt<T> getT(std::unique_ptr<Base> const& base) {
		return static_cast<TAsset<T>&>(*base).t.get();
	}

	Resources m_resources;
	ktl::strict_tmutex<TAssets> m_assets;
	mutable std::mutex m_reloadMutex;
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
	OnModified onModified;
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
	std::unique_ptr<T> t;

	static std::string_view tName() { return utils::removeNamespaces(utils::tName<T>()); }

	TAsset(std::string&& uri, std::unique_ptr<T>&& t) noexcept : Base(std::move(uri), tName(), Sign::make<T>(), {}), t(std::move(t)) {}
	TAsset(std::string&& uri, AssetLoadInfo<T>&& info, DoReload&& doReload) noexcept
		: Base(std::move(uri), tName(), Sign::make<T>(), std::move(doReload)), info(std::move(info)) {}
	bool dirty() const override { return info && info->modified(); }
};

template <typename T>
Opt<T> AssetStore::add(std::string uri, T t) {
	return add(std::move(uri), std::make_unique<T>(std::move(t)));
}

template <typename T>
Opt<T> AssetStore::add(std::string uri, std::unique_ptr<T>&& t) {
	return add(std::make_unique<TAsset<T>>(std::move(uri), std::move(t)));
}

template <typename T>
Opt<T> AssetStore::load(std::string uri, AssetLoadData<T> data) {
	AssetLoader<T> loader;
	AssetLoadInfo<T> info(this, &m_resources, std::move(data), uri);
	DoReload doReload;
	if constexpr (detail::reloadable_asset_v<T>) {
		doReload = [this](Base* base) { return this->reloadAsset(static_cast<TAsset<T>&>(*base)); };
	}
	auto tasset = std::make_unique<TAsset<T>>(std::move(uri), std::move(info), std::move(doReload));
	if ((tasset->t = loader.load(*tasset->info)); tasset->t) { return add(std::move(tasset)); }
	logW(LC_EndUser, "[Asset] Failed to load [{}]!", tasset->uri);
	return {};
}

template <typename T>
Opt<T> AssetStore::find(Hash uri) const {
	ktl::klock lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		EXPECT(it->second);
		if (it->second->sign == Sign::make<T>()) { return getT<T>(it->second); }
	}
	return {};
}

template <typename T>
bool AssetStore::exists(Hash uri) const noexcept {
	ktl::klock lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		EXPECT(it->second);
		if (it->second->sign == Sign::make<T>()) { return true; }
	}
	return false;
}

template <typename T>
	requires(detail::reloadable_asset_v<T>)
bool AssetStore::reload(Hash uri) {
	ktl::klock lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		EXPECT(it->second);
		if (it->second->sign == Sign::make<T>()) { return reloadAsset(static_cast<TAsset<T>&>(*it->second)); }
	}
	return false;
}

template <typename T>
bool AssetStore::unload(Hash uri) {
	ktl::klock lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		EXPECT(it->second);
		if (it->second->sign == Sign::make<T>()) {
			lock->erase(it);
			return true;
		}
	}
	return false;
}

template <template <typename...> typename L>
L<std::mutex> AssetStore::reloadLock() const {
	return L<std::mutex>(m_reloadMutex);
}

template <typename T>
Opt<T> AssetStore::add(std::unique_ptr<TAsset<T>>&& tasset) {
	if (!tasset) { return {}; }
	Hash const key = tasset->uri;
	auto const [it, b] = ktl::klock(m_assets)->insert_or_assign(key, std::move(tasset));
	if (!b) { logW(LC_EndUser, "[Asset] Overwriting [{}]!", it->second->uri); }
	logI(LC_EndUser, "== [Asset] [{}] added", it->second->uri);
	return getT<T>(it->second);
}

template <typename T>
	requires(detail::reloadable_asset_v<T>)
bool AssetStore::reloadAsset(TAsset<T>& out_asset) const {
	auto lock = reloadLock();
	AssetLoader<T> loader;
	if (loader.reload(*out_asset.t, *out_asset.info)) {
		logI(LC_LibUser, "== [Asset] [{}] reloaded", out_asset.uri);
		out_asset.onModified();
		return true;
	}
	return false;
}

template <typename T>
AssetStore::Sign AssetStore::sign() {
	return Sign::make<T>();
}

template <typename... T>
Span<AssetStore::Sign const> AssetStore::signs() {
	if constexpr (sizeof...(T) > 0) {
		thread_local Sign const ret[] = {Sign::make<T>()...};
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
