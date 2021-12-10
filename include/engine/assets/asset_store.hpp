#pragma once
#include <core/log_channel.hpp>
#include <core/std_types.hpp>
#include <core/utils/expect.hpp>
#include <core/utils/string.hpp>
#include <dens/detail/sign.hpp>
#include <engine/assets/asset.hpp>
#include <engine/assets/asset_loader.hpp>
#include <ktl/delegate.hpp>
#include <ktl/shared_tmutex.hpp>

namespace le {
class AssetStore : public NoCopy {
  public:
	using OnModified = ktl::delegate<>;
	using Sign = dens::detail::sign_t;

	struct Index;

	template <typename T>
	Asset<T> add(std::string uri, T t);
	template <typename T>
	Asset<T> add(std::string uri, std::unique_ptr<T>&& t);
	template <typename T>
	Asset<T> load(std::string uri, AssetLoadData<T> data);
	template <typename T>
	Asset<T> find(Hash uri) const;
	bool exists(Hash uri) const noexcept;
	template <typename T>
	bool exists(Hash uri) const noexcept;
	bool unload(Hash uri);
	template <typename T>
	bool unload(Hash uri);
	template <typename T>
		requires(detail::reloadable_asset_v<T>)
	bool reload(Hash uri);

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
	using DoUpdate = ktl::move_only_function<bool(Base*)>;
	using TAssets = std::unordered_map<Hash, std::unique_ptr<Base>>;
	template <typename T>
	struct TAsset;

	template <typename T>
		requires(detail::reloadable_asset_v<T>)
	bool reloadAsset(TAsset<T>& out_asset) const;

	template <typename T>
	static Asset<T> makeAsset(TAsset<T>& t) noexcept;

	Resources m_resources;
	ktl::shared_strict_tmutex<TAssets> m_assets;
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
	DoUpdate doUpdate;
	Sign sign;
	Base(std::string uri, std::string_view typeName, Sign sign, DoUpdate&& doUpdate) noexcept
		: uri(std::move(uri)), typeName(typeName), doUpdate(std::move(doUpdate)), sign(sign) {}
	virtual ~Base() = default;
};

template <typename T>
struct AssetStore::TAsset : Base {
	std::optional<AssetLoadInfo<T>> info;
	std::unique_ptr<T> t;

	static std::string_view tName() { return utils::removeNamespaces(utils::tName<T>()); }

	TAsset(std::string&& uri, std::unique_ptr<T>&& t) noexcept : Base(std::move(uri), tName(), Sign::make<T>(), {}), t(std::move(t)) {}
	TAsset(std::string&& uri, AssetLoadInfo<T>&& info, DoUpdate&& doUpdate) noexcept
		: Base(std::move(uri), tName(), Sign::make<T>(), std::move(doUpdate)), info(std::move(info)) {}
};

template <typename T>
Asset<T> AssetStore::add(std::string uri, T t) {
	return add(std::move(uri), std::make_unique<T>(std::move(t)));
}

template <typename T>
Asset<T> AssetStore::add(std::string uri, std::unique_ptr<T>&& t) {
	if (t) {
		ktl::unique_tlock<TAssets> lock(m_assets);
		Hash const key = uri;
		auto tasset = std::make_unique<TAsset<T>>(std::move(uri), std::move(t));
		TAsset<T>& ret = *tasset;
		auto const [it, b] = lock->emplace(key, std::move(tasset));
		if (!b) { logW(LC_EndUser, "[Asset] Overwriting [{}]!", ret.uri); }
		logI(LC_EndUser, "== [Asset] [{}] added", ret.uri);
		return makeAsset<T>(ret);
	}
	return {};
}

template <typename T>
Asset<T> AssetStore::load(std::string uri, AssetLoadData<T> data) {
	Hash const key = uri;
	AssetLoader<T> loader;
	AssetLoadInfo<T> info(this, &m_resources, std::move(data), uri);
	DoUpdate doUpdate;
	if constexpr (detail::reloadable_asset_v<T>) {
		doUpdate = [this](Base* base) {
			auto& asset = static_cast<TAsset<T>&>(*base);
			if (asset.t && asset.info && asset.info->modified()) { return this->reloadAsset(asset); }
			return false;
		};
	}
	auto tasset = std::make_unique<TAsset<T>>(std::move(uri), std::move(info), std::move(doUpdate));
	auto& ret = *tasset;
	if ((ret.t = loader.load(*ret.info)); ret.t) {
		logI(LC_EndUser, "== [Asset] [{}] loaded", ret.uri);
		ktl::unique_tlock<TAssets> lock(m_assets);
		lock->emplace(key, std::move(tasset));
		return makeAsset(ret);
	}
	logW(LC_EndUser, "[Asset] Failed to load [{}]!", ret.uri);
	return {};
}

template <typename T>
Asset<T> AssetStore::find(Hash uri) const {
	ktl::shared_tlock<TAssets const> lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		EXPECT(it->second);
		if (it->second->sign == Sign::make<T>()) { return makeAsset(static_cast<TAsset<T>&>(*it->second)); }
	}
	return {};
}

template <typename T>
bool AssetStore::exists(Hash uri) const noexcept {
	ktl::shared_tlock<TAssets const> lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		EXPECT(it->second);
		if (it->second->sign == Sign::make<T>()) { return true; }
	}
	return false;
}

template <typename T>
	requires(detail::reloadable_asset_v<T>)
bool AssetStore::reload(Hash uri) {
	ktl::shared_tlock<TAssets> lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		EXPECT(it->second);
		if (it->second->sign == Sign::make<T>()) { return reloadAsset(static_cast<TAsset<T>&>(*it->second)); }
	}
	return false;
}

template <typename T>
bool AssetStore::unload(Hash uri) {
	ktl::shared_tlock<TAssets> lock(m_assets);
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
Asset<T> AssetStore::makeAsset(TAsset<T>& t) noexcept {
	EXPECT(t.t);
	return {&*t.t, t.uri, &t.onModified};
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
