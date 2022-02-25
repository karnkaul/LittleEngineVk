#include <levk/core/utils/expect.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/skybox.hpp>
#include <levk/graphics/texture.hpp>

namespace le::graphics {
Skybox::Skybox(not_null<VRAM*> vram, Opt<Texture const> texture) : m_mesh(vram) {
	m_mesh.construct(makeCube());
	cubemap(texture);
}

void Skybox::cubemap(Opt<Texture const> texture) noexcept {
	if (texture) {
		EXPECT(texture->type() == Texture::Type::eCube);
		if (texture->type() != Texture::Type::eCube) { return; }
	}
	m_cubemap = texture;
}

DrawPrimitive Skybox::primitive() const noexcept {
	static BPMaterialData const s_mat{};
	MaterialTextures matTex;
	matTex[MatTexType::eDiffuse] = m_cubemap;
	return {matTex, &m_mesh, &s_mat};
}
} // namespace le::graphics
