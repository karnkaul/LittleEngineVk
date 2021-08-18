#pragma once
#include <engine/scene/prop.hpp>
#include <graphics/mesh.hpp>

namespace le {
namespace graphics {
class Texture;
}

class Skybox {
  public:
	using Cubemap = graphics::Texture;

	Skybox(not_null<Cubemap const*> cubemap);

	Prop const& prop() const noexcept;

	mutable Prop m_prop;
	graphics::Mesh m_cube;
	not_null<Cubemap const*> m_cubemap;
};
} // namespace le
