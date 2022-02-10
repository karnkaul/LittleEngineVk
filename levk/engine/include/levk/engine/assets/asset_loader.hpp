#pragma once
#include <ktl/delegate.hpp>
#include <levk/core/utils/algo.hpp>
#include <levk/engine/assets/resources.hpp>
#include <memory>
#include <vector>

namespace le {
class AssetStore;

template <typename T>
using AssetStorage = std::optional<T>;

template <typename T, typename... Args>
AssetStorage<T> makeAssetStorage(Args&&... args) {
	return T(std::forward<Args>(args)...);
}

///
/// \brief Customisation point
///
template <typename T>
struct AssetLoadData {
	io::Path uri;
};

template <typename T>
class AssetLoadInfo {
  public:
	using OnModified = ktl::delegate<>;

	template <typename Data>
	AssetLoadInfo(not_null<AssetStore const*> store, not_null<Resources*> resources, Data&& data, Hash uri);

	Resource const* resource(io::Path const& path, Resource::Type type, Resources::Flags flags) const;
	io::Media const& media() const;
	bool modified() const;

  public:
	AssetLoadData<T> m_data;
	not_null<AssetStore const*> m_store;
	Hash m_uri;

  private:
	not_null<Resources*> m_resources;
	mutable std::unordered_map<Hash, not_null<Resource const*>> m_monitors;
};

template <typename T>
struct AssetLoader {
	static constexpr bool specialized = false;

	std::unique_ptr<T> load(AssetLoadInfo<T> const& info) const;
	bool reload(T& out_t, AssetLoadInfo<T> const& info) const;
};

namespace detail {
template <typename T>
struct specialized_asset_loader : std::true_type {};
template <typename T>
	requires(!AssetLoader<T>::specialized)
struct specialized_asset_loader<T> : std::false_type {};
template <typename T>
static constexpr bool reloadable_asset_v = specialized_asset_loader<T>::value;
} // namespace detail

// impl

template <typename T>
template <typename Data>
AssetLoadInfo<T>::AssetLoadInfo(not_null<AssetStore const*> store, not_null<Resources*> resources, Data&& data, Hash uri)
	: m_data(std::forward<Data>(data)), m_store(store), m_uri(uri), m_resources(resources) {}
template <typename T>
Resource const* AssetLoadInfo<T>::resource(io::Path const& path, Resource::Type type, Resources::Flags flags) const {
	if (auto pRes = m_resources->load(path, type, flags)) {
		auto const pathStr = path.generic_string();
		if (flags.test(Resources::Flag::eMonitor) && !m_monitors.contains(pathStr)) { m_monitors.emplace(pathStr, pRes); }
		return pRes;
	}
	return nullptr;
}
template <typename T>
io::Media const& AssetLoadInfo<T>::media() const {
	return m_resources->media();
}
template <typename T>
bool AssetLoadInfo<T>::modified() const {
	for (auto const& [_, resource] : m_monitors) {
		if (auto status = resource->status(); status && *status == io::FileMonitor::Status::eModified) { return true; }
	}
	return false;
}
} // namespace le
