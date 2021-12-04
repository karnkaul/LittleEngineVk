#pragma once
#include <core/not_null.hpp>
#include <engine/render/material.hpp>
#include <engine/render/mesh_view.hpp>

namespace le {
namespace graphics {
class Texture;
}

using Cubemap = graphics::Texture;

class Skybox {
  public:
	Skybox(not_null<Cubemap const*> cubemap);

	MeshView mesh() const;

	mutable Material m_material;
	not_null<Cubemap const*> m_cubemap;
};
} // namespace le
