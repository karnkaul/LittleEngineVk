#pragma once
#include <array>
#include <memory>
#include <glm/glm.hpp>
#include <core/gdata.hpp>
#include <engine/assets/asset.hpp>
#include <engine/resources/resource_types.hpp>

namespace le::gfx
{
class Font final : public Asset
{
public:
	struct Glyph
	{
		u8 ch = '\0';
		glm::ivec2 st = glm::ivec2(0);
		glm::ivec2 uv = glm::ivec2(0);
		glm::ivec2 cell = glm::ivec2(0);
		glm::ivec2 offset = glm::ivec2(0);
		s32 xAdv = 0;
		s32 orgSizePt = 0;
		bool bBlank = false;

		void deserialise(u8 c, GData const& json);
	};
	struct Info final
	{
		res::Material::Inst material;
		stdfs::path sheetID;
		stdfs::path samplerID;
		stdfs::path materialID;
		std::vector<Glyph> glyphs;
		bytearray image;

		bool deserialise(GData const& json);
	};

	struct Text
	{
		enum class HAlign : s8
		{
			Centre = 0,
			Left,
			Right
		};

		enum class VAlign : s8
		{
			Middle = 0,
			Top,
			Bottom
		};

		std::string text;
		glm::vec3 pos = glm::vec3(0.0f);
		f32 scale = 1.0f;
		f32 nYPad = 0.2f;
		HAlign halign = HAlign::Centre;
		VAlign valign = VAlign::Middle;
		Colour colour = colours::white;
	};

public:
	static std::string const s_tName;

private:
	std::array<Glyph, maxVal<u8>()> m_glyphs;
	res::Material::Inst m_material;
	Glyph m_blankGlyph;
	res::Texture m_sheet;

public:
	Font(stdfs::path id, Info info);
	~Font();

public:
	Geometry generate(Text const& text) const;

public:
	Status update() override;

private:
	friend class Text2D;
};

class Text2D
{
public:
	struct Info
	{
		stdfs::path id;
		Font::Text data;
		Font* pFont = nullptr;
	};

protected:
	Font::Text m_data;
	res::Mesh m_mesh;
	Font* m_pFont = nullptr;

public:
	virtual ~Text2D();

public:
	bool setup(Info info);

	void updateText(Font::Text data);
	void updateText(std::string text);

	res::Mesh mesh() const;
	bool isReady() const;
	bool isBusy() const;
};
} // namespace le::gfx
