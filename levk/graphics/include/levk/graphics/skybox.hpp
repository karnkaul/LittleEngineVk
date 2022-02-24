#pragma once
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/mesh_view.hpp>

namespace le::graphics {
class Skybox {
  public:
	Skybox(not_null<VRAM*> vram, Opt<Texture const> texture = {});

	Opt<Texture const> cubemap() const noexcept { return m_cubemap; }
	void cubemap(Opt<Texture const> texture) noexcept;

	MeshView meshView() const noexcept;

  private:
	MeshPrimitive m_mesh;
	Opt<Texture const> m_cubemap{};
};
} // namespace le::graphics
