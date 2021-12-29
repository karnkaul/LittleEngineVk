#include <core/utils/expect.hpp>
#include <engine/render/layer.hpp>
#include <engine/render/material.hpp>
#include <engine/scene/asset_provider.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le {
MeshProvider MeshProvider::make(std::string primitiveURI, std::string materialURI) {
	MeshProvider ret;
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

void MeshProvider::uri(std::string assetURI) {
	m_assetURI = std::move(assetURI);
	m_hash = m_assetURI;
}

DynamicMesh DynamicMesh::make(GetMesh&& getMesh) {
	DynamicMesh ret;
	ret.m_getMesh = std::move(getMesh);
	return ret;
}
} // namespace le
