#include <engine/game/text2d.hpp>
#include <engine/resources/resources.hpp>
#include <resources/resources_impl.hpp>

namespace le {
bool Text2D::setup(Info info) {
	m_font = info.font;
	if (m_font.status() != res::Status::eReady) {
		auto font = res::find<res::Font>("fonts/default");
		if (!font) {
			return false;
		}
		m_font = *font;
	}
	m_data = std::move(info.data);
	auto fontInfo = res::info<res::Font>(m_font);
	if (auto pInfo = res::infoRW(m_mesh)) {
		pInfo->material = fontInfo.material;
		pInfo->material.tint = info.data.colour;
		m_mesh.resource.updateGeometry(m_font.generate(m_data));
	} else {
		m_mesh = {};
		res::Mesh::CreateInfo meshInfo;
		meshInfo.material = fontInfo.material;
		meshInfo.material.tint = info.data.colour;
		meshInfo.geometry = m_font.generate(m_data);
		meshInfo.type = res::Mesh::Type::eDynamic;
		m_mesh = res::load(info.id / "mesh", std::move(meshInfo));
	}
	auto const status = m_mesh.resource.status();
	return status != res::Status::eError && status != res::Status::eIdle;
}

void Text2D::updateText(res::Font::Text data) {
	if (m_mesh.ready() && m_font.status() == res::Status::eReady) {
		m_data = std::move(data);
		m_mesh.resource.updateGeometry(m_font.generate(m_data));
	}
	return;
}

void Text2D::updateText(std::string text) {
	if (m_mesh.ready() && m_font.status() == res::Status::eReady) {
		if (text != m_data.text) {
			m_data.text = std::move(text);
			updateText(m_data);
		}
	}
	return;
}

res::Mesh Text2D::mesh() const {
	return m_mesh.ready() ? m_mesh.resource : res::Mesh();
}

bool Text2D::ready() const {
	return m_mesh.ready() && m_font.status() == res::Status::eReady;
}

bool Text2D::busy() const {
	auto const mesh = m_mesh.resource.status();
	auto const font = m_font.status();
	return mesh == res::Status::eLoading || mesh == res::Status::eReloading || font == res::Status::eLoading || font == res::Status::eReloading;
}
} // namespace le
