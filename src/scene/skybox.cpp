#include <core/services.hpp>
#include <engine/engine.hpp>
#include <engine/scene/skybox.hpp>
#include <graphics/geometry.hpp>
#include <graphics/texture.hpp>

namespace le {
Skybox::Skybox(not_null<Cubemap const*> cubemap) : m_cube(Services::get<graphics::VRAM>()), m_cubemap(cubemap) {
	m_cube.construct(graphics::makeCube());
	ensure(m_cubemap->data().type == graphics::Texture::Type::eCube, "Invalid cubemap");
}

Prop const& Skybox::prop() const noexcept {
	m_prop.material.map_Kd = m_cubemap;
	m_prop.mesh = &m_cube;
	return m_prop;
}
} // namespace le
