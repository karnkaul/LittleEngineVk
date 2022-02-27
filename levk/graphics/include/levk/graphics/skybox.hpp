#pragma once
#include <levk/graphics/draw_primitive.hpp>
#include <levk/graphics/mesh_primitive.hpp>

namespace le::graphics {
class Skybox {
  public:
	Skybox(not_null<VRAM*> vram, Opt<Texture const> texture = {});

	Opt<Texture const> cubemap() const noexcept { return m_cubemap; }
	void cubemap(Opt<Texture const> texture) noexcept;

	DrawPrimitive drawPrimitive() const noexcept;

  private:
	Opt<Texture const> m_cubemap{};
	MeshPrimitive m_mesh;
};
} // namespace le::graphics
