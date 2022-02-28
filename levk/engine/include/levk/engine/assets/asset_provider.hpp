#pragma once
#include <levk/engine/assets/asset_store.hpp>

namespace le {
struct RenderLayer;
struct RenderLayer;
struct RenderPipeline;

template <typename T>
class AssetProvider {
  public:
	AssetProvider() = default;
	AssetProvider(Hash assetURI) noexcept : m_uri(assetURI) {}

	Hash uri() const noexcept { return m_uri; }
	void uri(Hash assetURI) noexcept { m_uri = assetURI; }

	bool empty() const noexcept { return m_uri == Hash(); }
	bool ready(AssetStore const& store) const;
	T const& get(AssetStore const& store, T const& fallback = T{}) const;
	Opt<T const> find(AssetStore const& store) const;

  private:
	Hash m_uri;
};

using RenderLayerProvider = AssetProvider<RenderLayer>;
using RenderPipeProvider = AssetProvider<RenderPipeline>;

// impl

template <typename T>
bool AssetProvider<T>::ready(AssetStore const& store) const {
	return store.exists<T>(m_uri);
}

template <typename T>
T const& AssetProvider<T>::get(AssetStore const& store, T const& fallback) const {
	if (auto t = find(store)) { return *t; }
	return fallback;
}

template <typename T>
Opt<T const> AssetProvider<T>::find(AssetStore const& store) const {
	return store.find<T>(m_uri);
}
} // namespace le
