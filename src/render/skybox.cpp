#include <core/services.hpp>
#include <engine/engine.hpp>
#include <engine/render/skybox.hpp>
#include <graphics/geometry.hpp>
#include <graphics/texture.hpp>

namespace le {
Skybox::Skybox(not_null<Cubemap const*> cubemap) : m_cube(Services::get<graphics::VRAM>()), m_cubemap(cubemap) {
	m_cube.construct(graphics::makeCube());
	ENSURE(m_cubemap->type() == graphics::Texture::Type::eCube, "Invalid cubemap");
}

Prop const& Skybox::prop() const noexcept {
	m_material.map_Kd = m_cubemap;
	m_prop = {&m_material, &m_cube};
	return m_prop;
}

MeshView Skybox::mesh() const {
	m_material.map_Kd = m_cubemap;
	return MeshObj{.primitive = &m_cube, .material = &m_material};
}
} // namespace le
