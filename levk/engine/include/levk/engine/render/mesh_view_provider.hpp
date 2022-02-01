#pragma once
#include <ktl/async/kfunction.hpp>
#include <levk/core/services.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/render/mesh_view.hpp>
#include <type_traits>

namespace le {
template <typename T>
concept MeshAPI = requires(T const& t) {
	{ t.mesh() } -> std::same_as<MeshView>;
};

class MeshProvider {
  public:
	using Sign = AssetStore::Sign;

	inline static std::string const default_material_v = "materials/default";

	template <MeshAPI T>
	static MeshProvider make(std::string assetURI);
	template <typename T, typename F>
	static MeshProvider make(std::string assetURI, F&& getMesh);
	static MeshProvider make(std::string primitiveURI, std::string materialURI = default_material_v);

	std::string_view assetURI() const noexcept { return m_assetURI; }
	std::string_view materialURI() const noexcept { return m_materialURI; }
	void uri(std::string assetURI);

	bool active() const noexcept { return m_getMesh.has_value(); }
	MeshView mesh() const { return m_getMesh ? m_getMesh(m_hash) : MeshView{}; }
	Sign sign() const noexcept { return m_sign; }

  private:
	using GetMesh = ktl::kfunction<MeshView(Hash)>;

	std::string m_assetURI;
	std::string m_materialURI;
	GetMesh m_getMesh;
	Hash m_hash;
	Sign m_sign{};
};

class DynamicMesh {
  public:
	using GetMesh = ktl::kfunction<MeshView()>;

	static DynamicMesh make(GetMesh&& getMesh);
	template <MeshAPI T>
	static DynamicMesh make(T const* source);

	bool active() const noexcept { return m_getMesh.has_value(); }
	MeshView mesh() const { return m_getMesh ? m_getMesh() : MeshView{}; }

  private:
	GetMesh m_getMesh;
};

// impl

template <typename T, typename F>
MeshProvider MeshProvider::make(std::string assetURI, F&& getMesh) {
	MeshProvider ret;
	ret.uri(std::move(assetURI));
	ret.m_sign = AssetStore::sign<T>();
	ret.m_getMesh = [gm = std::move(getMesh)](Hash assetURI) -> MeshView {
		if (auto store = Services::find<AssetStore>()) {
			if (auto asset = store->find<T>(assetURI)) { return gm(*asset); }
		}
		return MeshView{};
	};
	return ret;
}

template <MeshAPI T>
MeshProvider MeshProvider::make(std::string assetURI) {
	return make<T>(std::move(assetURI), [](T const& t) { return t.mesh(); });
}

template <MeshAPI T>
DynamicMesh DynamicMesh::make(T const* source) {
	EXPECT(source);
	return make([source]() { return source->mesh(); });
}
} // namespace le
