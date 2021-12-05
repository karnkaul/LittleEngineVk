#include <core/utils/expect.hpp>
#include <engine/render/draw_group.hpp>
#include <engine/render/material.hpp>
#include <engine/render/mesh_provider.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le {
MeshProvider MeshProvider::make(std::string primitiveURI, std::string materialURI) {
	MeshProvider ret;
	ret.uri(std::move(primitiveURI));
	Hash const mat = materialURI;
	ret.m_materialURI = std::move(materialURI);
	ret.m_sign = AssetStore::sign<MeshPrimitive>()[0];
	ret.m_getMesh = [mat](Hash primitiveURI) {
		if (auto store = Services::find<AssetStore>()) {
			auto const material = store->find<Material>(mat);
			auto const primitive = store->find<MeshPrimitive>(primitiveURI);
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

DrawGroupProvider DrawGroupProvider::make(std::string assetURI) {
	DrawGroupProvider ret;
	ret.uri(std::move(assetURI));
	return ret;
}

DrawGroup DrawGroupProvider::group() const {
	if (auto store = Services::find<AssetStore>()) {
		if (auto group = store->find<DrawGroup>(m_hash)) { return *group; }
	}
	return {};
}

void DrawGroupProvider::uri(std::string assetURI) {
	m_assetURI = std::move(assetURI);
	m_hash = m_assetURI;
}
} // namespace le
