#pragma once
#include <engine/render/mesh_view.hpp>
#include <engine/render/prop.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le {
namespace graphics {
class Texture;
}

using Cubemap = graphics::Texture;

class Skybox {
  public:
	Skybox(not_null<Cubemap const*> cubemap);

	Prop const& prop() const noexcept;
	MeshView mesh() const;

	mutable Material m_material;
	mutable Prop m_prop;
	graphics::MeshPrimitive m_cube;
	not_null<Cubemap const*> m_cubemap;
};
} // namespace le
