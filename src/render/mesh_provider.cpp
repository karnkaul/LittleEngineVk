#include <core/utils/expect.hpp>
#include <engine/render/material.hpp>
#include <engine/render/mesh_provider.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le {
MeshProvider MeshProvider::make(std::string primitiveURI, Hash const materialURI) {
	MeshProvider ret;
	ret.uri(std::move(primitiveURI));
	ret.m_getMesh = [materialURI](Hash primitiveURI) {
		if (auto store = Services::find<AssetStore>()) {
			auto const material = store->find<Material>(materialURI);
			auto const primitive = store->find<MeshPrimitive>(primitiveURI);
			EXPECT(material && primitive);
			if (material && primitive) { return MeshObj{.primitive = &*primitive, .material = &*material}; }
		}
		return MeshObj{};
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
