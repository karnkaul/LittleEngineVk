#include <levk/core/utils/expect.hpp>
#include <levk/engine/render/layer.hpp>
#include <levk/engine/render/material.hpp>
#include <levk/engine/render/mesh_view_provider.hpp>
#include <levk/graphics/mesh_primitive.hpp>

namespace le {
MeshViewProvider MeshViewProvider::make(std::string primitiveURI, std::string materialURI) {
	MeshViewProvider ret;
	ret.uri(std::move(primitiveURI));
	Hash const mat = materialURI;
	ret.m_materialURI = std::move(materialURI);
	ret.m_sign = AssetStore::sign<MeshPrimitive>();
	ret.m_getMesh = [mat](Hash primitiveURI) -> MeshView {
		if (auto store = Services::find<AssetStore>()) {
			auto const material = store->find<Material>(mat);
			auto const primitive = store->find<MeshPrimitive>(primitiveURI);
			if (material && primitive) { return MeshObj{.primitive = &*primitive, .material = &*material}; }
		}
		return {};
	};
	return ret;
}

void MeshViewProvider::uri(std::string assetURI) {
	m_assetURI = std::move(assetURI);
	m_hash = m_assetURI;
}

DynamicMeshView DynamicMeshView::make(GetMesh&& getMesh) {
	DynamicMeshView ret;
	ret.m_getMesh = std::move(getMesh);
	return ret;
}
} // namespace le
