#include <levk/core/utils/enumerate.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/engine/render/layer.hpp>
#include <levk/engine/render/primitive_provider.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/texture.hpp>

namespace le {
PrimitiveProvider::PrimitiveProvider(Hash meshPrimitiveURI, Hash materialURI) : m_meshURI(meshPrimitiveURI), m_materialURI(materialURI) {}

bool PrimitiveProvider::ready(AssetStore const& store) const {
	if (!store.exists<graphics::MeshPrimitive>(m_meshURI) || !store.exists<graphics::BPMaterialData>(m_materialURI)) { return false; }
	for (Hash const hash : m_textures.arr) {
		if (hash != Hash() && !store.exists<graphics::Texture>(hash)) { return false; }
	}
	return true;
}

bool PrimitiveProvider::addDrawPrimitives(AssetStore const& store, graphics::DrawList& out, glm::mat4 const& matrix) {
	auto mesh = store.find<graphics::MeshPrimitive>(m_meshURI);
	auto mat = store.find<graphics::BPMaterialData>(m_materialURI);
	if (!mesh || !mat) { return false; }
	graphics::MaterialTextures matTex;
	for (auto const [hash, idx] : utils::enumerate(m_textures.arr)) {
		if (hash != Hash()) {
			auto tex = store.find<graphics::Texture>(hash);
			if (!tex) { return false; }
			matTex[graphics::MatTexType(idx)] = tex;
		}
	}
	DrawPrimitive dp{matTex, mesh, mat};
	out.push(dp, matrix);
	return true;
}

// DynamicMeshView DynamicMeshView::make(GetMesh&& getMesh) {
// 	DynamicMeshView ret;
// 	ret.m_getMesh = std::move(getMesh);
// 	return ret;
// }
} // namespace le
