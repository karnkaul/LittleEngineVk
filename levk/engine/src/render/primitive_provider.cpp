#include <levk/core/utils/expect.hpp>
#include <levk/engine/render/layer.hpp>
#include <levk/engine/render/primitive_provider.hpp>
#include <levk/engine/render/texture_refs.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/texture.hpp>

namespace le {
PrimitiveProvider::PrimitiveProvider(Hash meshPrimitiveURI, Hash materialURI, Hash textureRefsURI)
	: m_meshURI(meshPrimitiveURI), m_materialURI(materialURI), m_texRefsURI(textureRefsURI) {}

bool PrimitiveProvider::addDrawPrimitives(AssetStore const& store, graphics::DrawList& out, glm::mat4 const& matrix) {
	auto const mesh = store.find<graphics::MeshPrimitive>(m_meshURI);
	auto const mat = store.find<graphics::BPMaterialData>(m_materialURI);
	if (!mesh || !mat) { return false; }
	graphics::MaterialTextures matTex;
	if (m_texRefsURI != Hash()) {
		if (auto refs = store.find<TextureRefs>(m_texRefsURI); !refs || !refs->fill(store, matTex)) { return false; }
	}
	DrawPrimitive dp{matTex, mesh, mat};
	out.push(dp, matrix);
	return true;
}
} // namespace le
