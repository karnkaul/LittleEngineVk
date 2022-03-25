#pragma once
#include <ktl/async/kmutex.hpp>
#include <ktl/hash_table.hpp>
#include <ktl/kunique_ptr.hpp>
#include <levk/core/hash.hpp>
#include <levk/core/io/fs_media.hpp>
#include <levk/core/log_channel.hpp>
#include <levk/core/not_null.hpp>
#include <levk/core/std_types.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/core/utils/string.hpp>
#include <levk/core/utils/type_guid.hpp>

namespace le {
class AssetStore : public NoCopy {
  public:
	using Sign = utils::TypeGUID;

	struct Index;

	template <typename T>
	Opt<T> add(std::string uri, T t);
	template <typename T>
	Opt<T> find(Hash uri) const;
	bool exists(Hash uri) const;
	template <typename T>
	bool exists(Hash uri) const;
	template <typename T>
	std::string_view uri(Hash hash) const;
	bool unload(Hash uri);
	template <typename T>
	bool unload(Hash uri);

	void checkModified();
	std::size_t size() const;
	void clear();

	io::Media const& media() const noexcept { return m_customMedia ? *m_customMedia : m_fsMedia; }
	io::FSMedia const& fsMedia() const noexcept { return m_fsMedia; }
	void customMedia(Opt<io::Media const> media) noexcept { m_customMedia = media; }

	template <typename T>
	static Sign sign();
	template <typename... T>
	static Span<Sign const> signs();

	template <typename... Ts>
	Index index(std::string_view filter = {}) const;
	Index index(Span<Sign const> signs, std::string_view filter) const;

  private:
	struct Base;
	using TAssets = ktl::hash_table<Hash, ktl::kunique_ptr<Base>>;
	template <typename T>
	struct TAsset;

	template <typename T>
	Opt<T> add(ktl::kunique_ptr<TAsset<T>>&& tasset);
	template <typename T>
	Opt<TAsset<T>> findImpl(Hash uri) const;

	template <typename T>
	static TAsset<T>& toTAsset(Base& base) noexcept;

	ktl::strict_tmutex<TAssets> m_assets;
	io::FSMedia m_fsMedia{};
	Opt<io::Media const> m_customMedia{};
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
	Sign sign;
	Base(std::string uri, std::string_view typeName, Sign sign) noexcept : uri(std::move(uri)), typeName(typeName), sign(sign) {}
	virtual ~Base() = default;
};

template <typename T>
struct AssetStore::TAsset : Base {
	using Storage = std::optional<T>;
	static Storage makeStorage(T&& t) { return Storage(std::move(t)); }

	Storage t;
	static std::string_view tName() { return utils::removeNamespaces(utils::tName<T>()); }
	TAsset(std::string&& uri, T&& t) noexcept : Base(std::move(uri), tName(), AssetStore::sign<T>()), t(makeStorage(std::move(t))) {}
};

template <typename T>
Opt<T> AssetStore::add(std::string uri, T t) {
	return add(ktl::make_unique<TAsset<T>>(std::move(uri), std::move(t)));
}

template <typename T>
Opt<T> AssetStore::find(Hash uri) const {
	auto t = findImpl<T>(uri);
	if (t && t->t) { return &*t->t; }
	return {};
}

template <typename T>
bool AssetStore::exists(Hash uri) const {
	return findImpl<T>(uri) != nullptr;
}

template <typename T>
std::string_view AssetStore::uri(Hash hash) const {
	if (auto t = findImpl<T>(hash)) { return t->uri; }
	return {};
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
Opt<T> AssetStore::add(ktl::kunique_ptr<TAsset<T>>&& tasset) {
	if (!tasset) { return {}; }
	Hash const key = tasset->uri;
	if (key == Hash()) { return {}; }
	ktl::klock lock(m_assets);
	auto const [it, b] = lock->insert_or_assign(key, std::move(tasset));
	if (!b) { logW(LC_EndUser, "[Asset] Overwriting [{}]!", it->second->uri); }
	logI(LC_EndUser, "== [Asset] [{}] added", it->second->uri);
	return &*toTAsset<T>(*it->second).t;
}

template <typename T>
auto AssetStore::findImpl(Hash uri) const -> Opt<TAsset<T>> {
	if (uri == Hash()) { return {}; }
	ktl::klock lock(m_assets);
	if (auto it = lock->find(uri); it != lock->end()) {
		EXPECT(it->second);
		if (it->second->sign == sign<T>()) { return &toTAsset<T>(*it->second); }
	}
	return {};
}

template <typename T>
auto AssetStore::toTAsset(Base& base) noexcept -> TAsset<T>& {
	return static_cast<TAsset<T>&>(base);
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
