#pragma once
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/mesh_view.hpp>

namespace le::graphics {
class Skybox {
  public:
	Skybox(not_null<VRAM*> vram, Opt<Texture const> texture = {});

	Opt<Texture const> cubemap() const noexcept { return m_materialTextures[MatTexType::eDiffuse]; }
	void cubemap(Opt<Texture const> texture) noexcept;

	PrimitiveView primitive() const noexcept;

  private:
	MaterialTextures m_materialTextures{};
	MeshPrimitive m_mesh;
};
} // namespace le::graphics
