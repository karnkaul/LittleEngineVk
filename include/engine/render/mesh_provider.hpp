#pragma once
#include <core/hash.hpp>
#include <core/services.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/render/mesh_view.hpp>
#include <ktl/move_only_function.hpp>
#include <type_traits>

namespace le {
template <typename T>
concept MeshAPI = requires(T const& t) {
	{ t.mesh() } -> std::same_as<MeshView>;
};

class MeshProvider {
  public:
	inline static Hash const default_material_v = "materials/default";

	template <MeshAPI T>
	static MeshProvider make(std::string assetURI);
	template <typename T, typename F>
	static MeshProvider make(std::string assetURI, F&& getMesh);
	static MeshProvider make(std::string primitiveURI, Hash materialURI = default_material_v);

	std::string const& uri() const noexcept { return m_assetURI; }
	void uri(std::string assetURI);

	bool active() const noexcept { return m_getMesh.has_value(); }
	MeshView mesh() const { return m_getMesh ? m_getMesh(m_hash) : MeshView{}; }

  private:
	using GetMesh = ktl::move_only_function<MeshView(Hash)>;

	std::string m_assetURI;
	GetMesh m_getMesh;
	Hash m_hash;
};

class DynamicMesh {
  public:
	using GetMesh = ktl::move_only_function<MeshView()>;

	static DynamicMesh make(GetMesh&& getMesh);
	template <MeshAPI T>
	static DynamicMesh make(not_null<T const*> source);

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
DynamicMesh DynamicMesh::make(not_null<T const*> source) {
	return make([source]() { return source->mesh(); });
}
} // namespace le
