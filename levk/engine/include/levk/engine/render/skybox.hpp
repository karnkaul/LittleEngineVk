#pragma once
#include <levk/core/not_null.hpp>
#include <levk/engine/render/material.hpp>
#include <levk/engine/scene/mesh_view.hpp>

namespace le {
namespace graphics {
class Texture;
}

class Skybox {
  public:
	Skybox(not_null<graphics::Texture const*> cubemap);

	MeshView mesh() const;

	mutable Material m_material;
	not_null<graphics::Texture const*> m_cubemap;
};
} // namespace le
