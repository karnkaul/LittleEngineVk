#include <levk/core/utils/enumerate.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/engine/render/layer.hpp>
#include <levk/engine/render/primitive_provider.hpp>
#include <levk/engine/render/texture_refs.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/texture.hpp>

namespace le {
PrimitiveProvider::PrimitiveProvider(Hash meshPrimitiveURI, Hash materialURI, Hash texturesURI)
	: m_meshURI(meshPrimitiveURI), m_materialURI(materialURI), m_matTexRefsURI(texturesURI) {}

bool PrimitiveProvider::addDrawPrimitives(AssetStore const& store, graphics::DrawList& out, glm::mat4 const& matrix) {
	auto const mesh = store.find<graphics::MeshPrimitive>(m_meshURI);
	auto const mat = store.find<graphics::BPMaterialData>(m_materialURI);
	auto const matTexRefs = store.find<TextureRefs>(m_matTexRefsURI);
	if (!mesh || !mat) { return false; }
	if (m_matTexRefsURI != Hash() && !matTexRefs) { return false; }
	graphics::MaterialTextures matTex;
	if (matTexRefs && !matTexRefs->fill(store, matTex)) { return false; }
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