#pragma once
#include <ktl/async/kfunction.hpp>
#include <levk/core/hash.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <type_traits>

namespace le {
struct RenderLayer;
struct RenderLayer;
struct RenderPipeline;

template <typename T>
class AssetProvider {
  public:
	AssetProvider() = default;
	AssetProvider(std::string uri) noexcept { this->uri(std::move(uri)); }

	std::string_view uri() const noexcept { return m_uri; }
	void uri(std::string assetURI) noexcept;

	bool empty() const noexcept { return m_hash == Hash(); }
	bool ready(AssetStore const& store) const;
	T const& get(AssetStore const& store, T const& fallback = T{}) const;

  private:
	std::string m_uri;
	Hash m_hash;
};

using RenderLayerProvider = AssetProvider<RenderLayer>;
using RenderPipeProvider = AssetProvider<RenderPipeline>;

// impl

template <typename T>
bool AssetProvider<T>::ready(AssetStore const& store) const {
	return store.exists<T>(m_hash);
}

template <typename T>
T const& AssetProvider<T>::get(AssetStore const& store, T const& fallback) const {
	if (auto t = store.find<T>(m_hash)) { return *t; }
	return fallback;
}

template <typename T>
void AssetProvider<T>::uri(std::string assetURI) noexcept {
	m_uri = std::move(assetURI);
	m_hash = m_uri;
}
} // namespace le
