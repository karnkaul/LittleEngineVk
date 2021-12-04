#include <core/services.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/engine.hpp>
#include <engine/render/skybox.hpp>
#include <engine/utils/logger.hpp>
#include <graphics/geometry.hpp>
#include <graphics/texture.hpp>

namespace le {
Skybox::Skybox(not_null<Cubemap const*> cubemap) : m_cube(Services::get<graphics::VRAM>()), m_cubemap(cubemap) {
	m_cube.construct(graphics::makeCube());
	ENSURE(m_cubemap->type() == graphics::Texture::Type::eCube, "Invalid cubemap");
}

MeshView Skybox::mesh() const {
	if (m_cubemap->ready()) {
		m_material.map_Kd = m_cubemap;
	} else {
		utils::g_log.log(dl::level::warn, 2, "[Skybox] Cubemap not ready");
		m_material.map_Kd = Services::get<AssetStore>()->find<graphics::Texture>("cubemaps/blank").peek();
	}
	return MeshObj{.primitive = &m_cube, .material = &m_material};
}
} // namespace le
