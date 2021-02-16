#pragma once
#include <vector>
#include <core/delegate.hpp>
#include <core/utils/algo.hpp>
#include <engine/assets/resources.hpp>

namespace le {
class AssetStore;
template <typename T>
class Asset;

///
/// \brief Customisation point
///
template <typename T>
struct AssetLoadData {
	io::Path id;
};

template <typename T>
class AssetLoadInfo {
  public:
	using OnModified = Delegate<>;

	template <typename Data>
	AssetLoadInfo(AssetStore const& store, Resources& resources, OnModified& onModified, Data&& data, Hash id);

	Resource const* resource(io::Path const& path, Resource::Type type, bool bMonitor, bool bForceReload = false) const;
	io::Reader const& reader() const;
	bool modified() const;
	void forceDirty(bool bDirty) const noexcept;

	template <typename U>
	void reloadDepend(Asset<U>& out_asset) const; // impl in asset_store.hpp; must include to instantiate!

  public:
	AssetLoadData<T> m_data;
	Ref<AssetStore const> m_store;
	Ref<OnModified> m_onModified;
	Hash m_id;

  private:
	Ref<Resources> m_resources;
	mutable std::unordered_map<Hash, Ref<Resource const>> m_monitors;
	mutable std::vector<OnModified::Tk> m_tokens;
	mutable bool m_bDirty = false;
};

template <typename T>
struct AssetLoader {
	std::optional<T> load(AssetLoadInfo<T> const& info) const;
	bool reload(T& out_t, AssetLoadInfo<T> const& info) const;
};

// impl

template <typename T>
template <typename Data>
AssetLoadInfo<T>::AssetLoadInfo(AssetStore const& store, Resources& resources, OnModified& onModified, Data&& data, Hash id)
	: m_data(std::forward<Data>(data)), m_store(store), m_onModified(onModified), m_id(id), m_resources(resources) {
}
template <typename T>
Resource const* AssetLoadInfo<T>::resource(io::Path const& path, Resource::Type type, bool bMonitor, bool bForceReload) const {
	if (auto pRes = m_resources.get().load(path, type, bMonitor, bForceReload)) {
		auto const pathStr = path.generic_string();
		if (bMonitor && utils::contains(m_monitors, pathStr)) {
			m_monitors.emplace(pathStr, *pRes);
		}
		return pRes;
	}
	return nullptr;
}
template <typename T>
io::Reader const& AssetLoadInfo<T>::reader() const {
	return m_resources.get().reader();
}
template <typename T>
bool AssetLoadInfo<T>::modified() const {
	if (m_bDirty) {
		return true;
	}
	for (auto const& [_, resource] : m_monitors) {
		if (auto status = resource.get().status(); status && *status == io::FileMonitor::Status::eModified) {
			return true;
		}
	}
	return false;
}
template <typename T>
void AssetLoadInfo<T>::forceDirty(bool bDirty) const noexcept {
	m_bDirty = bDirty;
}

template <typename T>
bool AssetLoader<T>::reload([[maybe_unused]] T& out_t, [[maybe_unused]] AssetLoadInfo<T> const& info) const {
	return false;
}
} // namespace le
