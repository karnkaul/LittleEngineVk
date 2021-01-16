#pragma once
#include <vector>
#include <core/delegate.hpp>
#include <engine/assets/resources.hpp>

namespace le {
class AssetStore;

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
	AssetLoadInfo(AssetStore const& store, Resources& resources, OnModified& onModified, Data&& data);

	Resource const* resource(io::Path const& path, Resource::Type type, bool bMonitor) const;
	io::Reader const& reader() const;
	bool modified() const;

  public:
	AssetLoadData<T> m_data;
	Ref<AssetStore const> m_store;
	Ref<OnModified> m_onModified;

  private:
	Ref<Resources> m_resources;
	mutable std::unordered_map<Hash, Ref<Resource const>> m_monitors;
};

template <typename T>
struct AssetLoader {
	std::optional<T> load(AssetLoadInfo<T> const& info) const;
	bool reload(T& out_t, AssetLoadInfo<T> const& info) const;
};

// impl

template <typename T>
template <typename Data>
AssetLoadInfo<T>::AssetLoadInfo(AssetStore const& store, Resources& resources, OnModified& onModified, Data&& data)
	: m_data(std::forward<Data>(data)), m_store(store), m_onModified(onModified), m_resources(resources) {
}

template <typename T>
Resource const* AssetLoadInfo<T>::resource(io::Path const& path, Resource::Type type, bool bMonitor) const {
	if (auto pRes = m_resources.get().load(path, type, bMonitor)) {
		auto const pathStr = path.generic_string();
		if (bMonitor && m_monitors.find(pathStr) == m_monitors.end()) {
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
	bool bRet = false;
	for (auto const& [_, resource] : m_monitors) {
		if (auto status = resource.get().status()) {
			bRet |= *status == io::FileMonitor::Status::eModified;
		}
	}
	return bRet;
}

template <typename T>
bool AssetLoader<T>::reload([[maybe_unused]] T& out_t, [[maybe_unused]] AssetLoadInfo<T> const& info) const {
	return false;
}
} // namespace le
