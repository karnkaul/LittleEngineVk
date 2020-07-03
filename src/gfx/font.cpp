#include <core/log.hpp>
#include <core/utils.hpp>
#include <engine/assets/resources.hpp>
#include <engine/gfx/font.hpp>
#include <engine/gfx/shader.hpp>
#include <engine/gfx/texture.hpp>

namespace le::gfx
{
namespace
{
glm::vec2 textTLOffset(Font::Text::HAlign h, Font::Text::VAlign v)
{
	glm::vec2 textTLoffset = glm::vec2(0.0f);
	switch (h)
	{
	case Font::Text::HAlign::Centre:
	{
		textTLoffset.x = -0.5f;
		break;
	}
	case Font::Text::HAlign::Left:
	default:
		break;

	case Font::Text::HAlign::Right:
	{
		textTLoffset.x = -1.0f;
		break;
	}
	}
	switch (v)
	{
	case Font::Text::VAlign::Middle:
	{
		textTLoffset.y = 0.5f;
		break;
	}
	default:
	case Font::Text::VAlign::Top:
	{
		break;
	}
	case Font::Text::VAlign::Bottom:
	{
		textTLoffset.y = 1.0f;
		break;
	}
	}
	return textTLoffset;
}
} // namespace

void Font::Glyph::deserialise(u8 c, GData const& json)
{
	ch = c;
	st = {json.getS32("x"), json.getS32("y")};
	cell = {json.getS32("width"), json.getS32("height")};
	uv = cell;
	offset = {json.getS32("originX"), json.getS32("originY")};
	xAdv = json.contains("advance") ? json.getS32("advance") : cell.x;
	orgSizePt = json.getS32("size");
	bBlank = json.getBool("isBlank");
}

bool Font::Info::deserialise(GData const& json)
{
	sheetID = json.getString("sheetID");
	samplerID = json.contains("sampler") ? json.getString("sampler") : "samplers/font";
	materialID = json.contains("material") ? json.getString("material") : "materials/default";
	auto glyphsData = json.getData("glyphs");
	for (auto& [key, value] : glyphsData.allFields())
	{
		if (!key.empty())
		{
			Glyph glyph;
			GData data;
			[[maybe_unused]] bool const bSuccess = data.read(std::move(value));
			ASSERT(bSuccess, "Failed to extract glyph!");
			glyph.deserialise((u8)key.at(0), data);
			if (glyph.cell.x > 0 && glyph.cell.y > 0)
			{
				glyphs.push_back(std::move(glyph));
			}
			else
			{
				LOG_W("[{}] Could not deserialise Glyph '{}'!", Font::s_tName, key.at(0));
			}
		}
	}
	return true;
}

std::string const Font::s_tName = utils::tName<Font>();

Font::Font(stdfs::path id, Info info) : Asset(std::move(id))
{
	Texture::Info sheetInfo;
	stdfs::path texID = m_id;
	texID += "_sheet";
	if (info.samplerID.empty())
	{
		info.samplerID = "samplers/font";
	}
	sheetInfo.samplerID = info.samplerID;
	sheetInfo.bytes = {std::move(info.image)};
	m_uSheet = std::make_unique<Texture>(std::move(texID), std::move(sheetInfo));
	if (m_uSheet->currentStatus() == Status::eError)
	{
		m_status = Status::eError;
		return;
	}
	m_uSheet->setup();
	glm::ivec2 maxCell = glm::vec2(0);
	s32 maxXAdv = 0;
	for (auto const& glyph : info.glyphs)
	{
		ASSERT(glyph.ch != '\0' && m_glyphs[(std::size_t)glyph.ch].ch == '\0', "Invalid/duplicate glyph!");
		m_glyphs.at((std::size_t)glyph.ch) = glyph;
		maxCell.x = std::max(maxCell.x, glyph.cell.x);
		maxCell.y = std::max(maxCell.y, glyph.cell.y);
		maxXAdv = std::max(maxXAdv, glyph.xAdv);
		if (glyph.bBlank)
		{
			m_blankGlyph = glyph;
		}
	}
	if (m_blankGlyph.xAdv == 0)
	{
		m_blankGlyph.cell = maxCell;
		m_blankGlyph.xAdv = maxXAdv;
	}
	m_material = info.material;
	if (!m_material.pMaterial)
	{
		m_material.pMaterial = Resources::inst().get<Material>("materials/default");
	}
	ASSERT(m_material.pMaterial, "Material is null!");
	m_material.pDiffuse = m_uSheet.get();
	m_material.flags.set({Material::Flag::eTextured, Material::Flag::eUI, Material::Flag::eDropColour});
	m_material.flags.reset({Material::Flag::eOpaque, Material::Flag::eLit});
	m_status = Status::eLoading;
}

Geometry Font::generate(Text const& text) const
{
	if (text.text.empty() || !m_uSheet || !isReady())
	{
		return {};
	}
	glm::ivec2 maxCell = glm::vec2(0);
	for (auto c : text.text)
	{
		maxCell.x = std::max(maxCell.x, m_glyphs.at((std::size_t)c).cell.x);
		maxCell.y = std::max(maxCell.y, m_glyphs.at((std::size_t)c).cell.y);
	}
	u32 lineCount = 1;
	for (std::size_t idx = 0; idx < text.text.size(); ++idx)
	{
		if (text.text[idx] == '\n')
		{
			++lineCount;
		}
	}
	f32 const lineHeight = ((f32)maxCell.y) * text.scale;
	f32 const linePad = lineHeight * text.nYPad;
	f32 const textHeight = (f32)lineCount * lineHeight;
	glm::vec2 const realTopLeft = text.pos;
	glm::vec2 textTL = realTopLeft;
	std::size_t nextLineIdx = 0;
	s32 yIdx = 0;
	f32 xPos = 0.0f;
	f32 lineWidth = 0.0f;
	auto const textTLoffset = textTLOffset(text.halign, text.valign);
	auto updateTextTL = [&]() {
		lineWidth = 0.0f;
		for (; nextLineIdx < text.text.size(); ++nextLineIdx)
		{
			auto const ch = text.text.at(nextLineIdx);
			if (ch == '\n')
			{
				break;
			}
			else
			{
				lineWidth += (f32)m_glyphs.at((std::size_t)ch).xAdv;
			}
		}
		lineWidth *= text.scale;
		++nextLineIdx;
		xPos = 0.0f;
		textTL = realTopLeft + textTLoffset * glm::vec2(lineWidth, textHeight);
		textTL.y -= (lineHeight + ((f32)yIdx * (lineHeight + linePad)));
	};
	updateTextTL();

	Geometry ret;
	u32 quadCount = (u32)text.text.length();
	ret.reserve(4 * quadCount, 6 * quadCount);
	auto const normal = glm::vec3(0.0f);
	auto const colour = glm::vec3(1.0f);
	auto const texSize = m_uSheet->m_size;
	for (auto const c : text.text)
	{
		if (c == '\n')
		{
			++yIdx;
			updateTextTL();
			continue;
		}
		auto const& search = m_glyphs.at((std::size_t)c);
		auto const& glyph = search.ch == '\0' ? m_blankGlyph : search;
		auto const offset = glm::vec3(xPos - (f32)glyph.offset.x * text.scale, (f32)glyph.offset.y * text.scale, 0.0f);
		auto const tl = glm::vec3(textTL.x, textTL.y, text.pos.z) + offset;
		auto const s = (f32)glyph.st.x / (f32)texSize.x;
		auto const t = (f32)glyph.st.y / (f32)texSize.y;
		auto const u = s + (f32)glyph.uv.x / (f32)texSize.x;
		auto const v = t + (f32)glyph.uv.y / (f32)texSize.y;
		glm::vec2 const cell = {(f32)glyph.cell.x * text.scale, (f32)glyph.cell.y * text.scale};
		auto const v0 = ret.addVertex({tl, colour, normal, glm::vec2(s, t)});
		auto const v1 = ret.addVertex({tl + glm::vec3(cell.x, 0.0f, 0.0f), colour, normal, glm::vec2(u, t)});
		auto const v2 = ret.addVertex({tl + glm::vec3(cell.x, -cell.y, 0.0f), colour, normal, glm::vec2(u, v)});
		auto const v3 = ret.addVertex({tl + glm::vec3(0.0f, -cell.y, 0.0f), colour, normal, glm::vec2(s, v)});
		ret.addIndices({v0, v1, v2, v2, v3, v0});
		xPos += ((f32)glyph.xAdv * text.scale);
	}
	return ret;
}

Asset::Status Font::update()
{
	if (!m_uSheet)
	{
		m_status = Status::eMoved;
		return m_status;
	}
	m_status = m_uSheet->update();
	return m_status;
}

bool Text2D::setup(Info info)
{
	m_pFont = info.pFont;
	if (!m_pFont)
	{
		m_pFont = Resources::inst().get<Font>("fonts/default");
	}
	ASSERT(m_pFont, "Font is null!");
	if (!m_pFont)
	{
		return false;
	}
	m_data = std::move(info.data);
	Mesh::Info meshInfo;
	stdfs::path meshID = info.id;
	meshID += "_mesh";
	meshInfo.material = m_pFont->m_material;
	meshInfo.geometry = m_pFont->generate(m_data);
	meshInfo.type = Mesh::Type::eDynamic;
	m_uMesh = std::make_unique<Mesh>(std::move(meshID), std::move(meshInfo));
	if (m_uMesh->currentStatus() != Asset::Status::eError)
	{
		m_uMesh->m_material = m_pFont->m_material;
		m_uMesh->m_material.tint = info.data.colour;
		m_uMesh->setup();
		return true;
	}
	return false;
}

void Text2D::updateText(Font::Text data)
{
	if (m_uMesh->isReady() && m_pFont && m_pFont->isReady())
	{
		m_data = std::move(data);
		m_uMesh->updateGeometry(m_pFont->generate(m_data));
	}
	return;
}

void Text2D::updateText(std::string text)
{
	if (m_uMesh->isReady() && m_pFont && m_pFont->isReady())
	{
		if (text != m_data.text)
		{
			m_data.text = std::move(text);
			updateText(m_data);
		}
	}
	return;
}

Mesh const* Text2D::mesh() const
{
	m_uMesh->update();
	return isReady() ? m_uMesh.get() : nullptr;
}

bool Text2D::isReady() const
{
	return m_uMesh->isReady() && m_pFont && m_pFont->isReady();
}

bool Text2D::isBusy() const
{
	return m_uMesh->isBusy() || !m_pFont || m_pFont->isBusy();
}
} // namespace le::gfx
