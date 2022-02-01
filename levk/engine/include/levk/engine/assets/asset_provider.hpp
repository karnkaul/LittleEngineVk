#pragma once
#include <ktl/async/kfunction.hpp>
#include <levk/core/hash.hpp>
#include <levk/core/services.hpp>
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
	bool ready() const;
	T const& get(T const& fallback = T{}) const;

  private:
	std::string m_uri;
	Hash m_hash;
};

using RenderLayerProvider = AssetProvider<RenderLayer>;
using RenderPipeProvider = AssetProvider<RenderPipeline>;

// impl

template <typename T>
bool AssetProvider<T>::ready() const {
	if (auto store = Services::find<AssetStore>()) { return store->exists<T>(m_hash); }
	return false;
}

template <typename T>
T const& AssetProvider<T>::get(T const& fallback) const {
	if (auto store = Services::find<AssetStore>()) {
		if (auto t = store->find<T>(m_hash)) { return *t; }
	}
	return fallback;
}

template <typename T>
void AssetProvider<T>::uri(std::string assetURI) noexcept {
	m_uri = std::move(assetURI);
	m_hash = m_uri;
}
} // namespace le
