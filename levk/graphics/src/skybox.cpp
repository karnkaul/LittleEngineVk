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
	EXPECT(texture->type() == Texture::Type::eCube);
	if (texture->type() != Texture::Type::eCube) { return; }
	m_materialTextures[MatTexType::eDiffuse] = texture;
}

PrimitiveView Skybox::primitive() const noexcept {
	static BPMaterialData const s_mat{};
	return {&m_mesh, &m_materialTextures, &s_mat};
}
} // namespace le::graphics
