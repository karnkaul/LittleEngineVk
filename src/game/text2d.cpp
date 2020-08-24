#include <engine/game/text2d.hpp>
#include <engine/resources/resources.hpp>

namespace le
{
Text2D::Text2D() = default;
Text2D::Text2D(Text2D&&) = default;
Text2D& Text2D::operator=(Text2D&&) = default;

Text2D::~Text2D()
{
	res::unload(m_mesh);
}

bool Text2D::setup(Info info)
{
	m_font = info.font;
	if (m_font.guid == res::GUID::s_null)
	{
		auto font = res::find<res::Font>("fonts/default");
		if (!font)
		{
			return false;
		}
		m_font = *font;
	}
	m_data = std::move(info.data);
	auto fontInfo = res::info(m_font);
	res::Mesh::CreateInfo meshInfo;
	meshInfo.material = fontInfo.material;
	meshInfo.material.tint = info.data.colour;
	meshInfo.geometry = m_font.generate(m_data);
	meshInfo.type = res::Mesh::Type::eDynamic;
	m_mesh = res::load(info.id / "mesh", std::move(meshInfo));
	if (m_mesh.status() != res::Status::eError)
	{
		return true;
	}
	return false;
}

void Text2D::updateText(res::Font::Text data)
{
	if (m_mesh.status() == res::Status::eReady && m_font.status() == res::Status::eReady)
	{
		m_data = std::move(data);
		m_mesh.updateGeometry(m_font.generate(m_data));
	}
	return;
}

void Text2D::updateText(std::string text)
{
	if (m_mesh.status() == res::Status::eReady && m_font.status() == res::Status::eReady)
	{
		if (text != m_data.text)
		{
			m_data.text = std::move(text);
			updateText(m_data);
		}
	}
	return;
}

res::Mesh Text2D::mesh() const
{
	return m_mesh.status() == res::Status::eReady ? m_mesh : res::Mesh();
}

bool Text2D::isReady() const
{
	return m_mesh.status() == res::Status::eReady && m_font.status() == res::Status::eReady;
}

bool Text2D::isBusy() const
{
	auto const mesh = m_mesh.status();
	auto const font = m_font.status();
	return mesh == res::Status::eLoading || mesh == res::Status::eReloading || font == res::Status::eLoading || font == res::Status::eReloading;
}
} // namespace le
