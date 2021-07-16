#pragma once
#include <engine/scene/primitive.hpp>
#include <graphics/mesh.hpp>

namespace le {
namespace graphics {
class Texture;
}

class Skybox {
  public:
	using Cubemap = graphics::Texture;

	Skybox(not_null<Cubemap const*> cubemap);

	Primitive const& primitive() const noexcept;

	mutable Primitive m_primitive;
	graphics::Mesh m_cube;
	not_null<Cubemap const*> m_cubemap;
};
} // namespace le
