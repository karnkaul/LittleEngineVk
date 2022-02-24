#include <levk/core/services.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/render/skybox.hpp>
#include <levk/graphics/geometry.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/texture.hpp>

namespace le {
Skybox::Skybox(not_null<graphics::Texture const*> cubemap) : m_cubemap(cubemap) {
	ENSURE(m_cubemap->type() == graphics::Texture::Type::eCube, "Invalid cubemap");
}

MeshView Skybox::mesh() const {
	if (auto store = Services::find<AssetStore>()) {
		auto cube = store->find<MeshPrimitive>("meshes/cube");
		if (!cube) { return {}; }
		if (!m_cubemap->ready()) { return {}; }
		m_material.map_Kd = m_cubemap;
		return MeshObj{.primitive = &*cube, .material = &m_material};
	}
	return {};
}
} // namespace le
