#pragma once
#include <levk/graphics/draw_primitive.hpp>
#include <levk/graphics/mesh_primitive.hpp>

namespace le::graphics {
class Skybox {
  public:
	Skybox(not_null<VRAM*> vram, Opt<Texture const> texture = {});

	Opt<Texture const> cubemap() const noexcept { return m_cubemap; }
	void cubemap(Opt<Texture const> texture) noexcept;

	DrawPrimitive primitive() const noexcept;

  private:
	Opt<Texture const> m_cubemap{};
	MeshPrimitive m_mesh;
};

template <>
struct AddDrawPrimitives<Skybox> {
	template <std::output_iterator<DrawPrimitive> It>
	void operator()(Skybox const& skybox, It it) const {
		if (auto primitive = skybox.primitive()) { *it++ = skybox.primitive(); }
	}
};
} // namespace le::graphics
