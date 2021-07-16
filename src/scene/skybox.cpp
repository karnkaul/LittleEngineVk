#include <core/services.hpp>
#include <engine/engine.hpp>
#include <engine/scene/skybox.hpp>
#include <graphics/geometry.hpp>
#include <graphics/texture.hpp>

namespace le {
Skybox::Skybox(not_null<Cubemap const*> cubemap) : m_cube(Services::locate<graphics::VRAM>()), m_cubemap(cubemap) {
	m_cube.construct(graphics::makeCube());
	ensure(m_cubemap->data().type == graphics::Texture::Type::eCube, "Invalid cubemap");
}

Primitive const& Skybox::primitive() const noexcept {
	m_primitive.material.map_Kd = m_cubemap;
	m_primitive.mesh = &m_cube;
	return m_primitive;
}
} // namespace le
