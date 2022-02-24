#pragma once
#include <levk/core/not_null.hpp>
#include <levk/engine/render/material.hpp>
#include <levk/engine/render/mesh_view.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/mesh_view.hpp>

namespace le {
namespace graphics {
class Texture;
}

class Skybox {
  public:
	Skybox(not_null<graphics::Texture const*> cubemap);

	MeshView mesh() const;
	graphics::MeshView meshView() const;

	mutable Material m_material;
	graphics::BPMaterialData m_materialData;
	not_null<graphics::Texture const*> m_cubemap;
};
} // namespace le
