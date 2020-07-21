#pragma once
#include <vector>
#include <glm/vec2.hpp>
#include <core/colour.hpp>
#include <core/flags.hpp>
#include <core/gdata.hpp>
#include <core/hash.hpp>
#include <core/utils.hpp>
#include <core/zero.hpp>
#include <engine/gfx/geometry.hpp>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_RESOURCES_HOT_RELOAD)
#define LEVK_RESOURCES_HOT_RELOAD
#endif
#endif

namespace le::res
{
using GUID = TZero<u64>;

enum class Status : s8
{
	eIdle,
	eReady,		// ready to use
	eLoading,	// transferring resources
	eReloading, // reloading (only relevant if `LEVK_RESOURCES_HOT_RELOAD` is defined)
	eError,		// cannot be used
	eCOUNT_
};

struct Resource
{
	GUID guid;
};

struct Shader final : Resource
{
	enum class Type : s8;
	struct Info;
	struct CreateInfo;

	struct Impl;

	static std::string const s_tName;

	Info const& info() const;
	Status status() const;
};

struct Sampler final : Resource
{
	enum class Filter : s8;
	enum class Mode : s8;
	struct Info;
	struct CreateInfo;

	struct Impl;

	static std::string const s_tName;

	Info const& info() const;
	Status status() const;
};

struct Texture final : Resource
{
	enum class Space : s8;
	enum class Type : s8;
	struct Raw;
	struct Info;
	struct CreateInfo;

	struct Impl;

	static std::string const s_tName;

	Info const& info() const;
	Status status() const;
};

struct Material final : Resource
{
	enum class Flag : s8
	{
		eTextured,
		eLit,
		eOpaque,
		eDropColour,
		eUI,
		eSkybox,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	struct Inst;
	struct Info;
	struct CreateInfo;

	struct Impl;

	static std::string const s_tName;

	Info const& info() const;
	Status status() const;
};

struct Mesh final : Resource
{
	enum class Type : s8;
	struct Info;
	struct CreateInfo;

	struct Impl;

	static std::string const s_tName;

	Info const& info() const;
	Status status() const;

	void updateGeometry(gfx::Geometry geometry);
	Material::Inst& material();
};

struct Font final : Resource
{
	struct Glyph;
	struct Info;
	struct CreateInfo;
	struct Text;

	struct Impl;

	static std::string const s_tName;

	Info const& info() const;
	Status status() const;

	gfx::Geometry generate(Text const& text) const;
};

struct InfoBase
{
	stdfs::path id;
};

enum class Shader::Type : s8
{
	eVertex,
	eFragment,
	eCOUNT_
};
struct Shader::Info : InfoBase
{
};
struct Shader::CreateInfo
{
	EnumArray<bytearray, Type> codeMap;
	EnumArray<stdfs::path, Type> codeIDMap;
};

enum class Sampler::Filter : s8
{
	eLinear,
	eNearest,
	eCOUNT_
};
enum class Sampler::Mode : s8
{
	eRepeat,
	eClampEdge,
	eClampBorder,
	eCOUNT_
};
struct Sampler::Info : InfoBase
{
	Filter min = Filter::eLinear;
	Filter mag = Filter::eLinear;
	Filter mip = Filter::eLinear;
	Mode mode = Mode::eRepeat;
};
struct Sampler::CreateInfo
{
	Filter min = Filter::eLinear;
	Filter mag = Filter::eLinear;
	Filter mip = Filter::eLinear;
	Mode mode = Mode::eRepeat;
};

enum class Texture::Space : s8
{
	eSRGBNonLinear,
	eRGBLinear,
	eCOUNT_
};
enum class Texture::Type : s8
{
	e2D,
	eCube,
	eCOUNT_
};
struct Texture::Raw
{
	Span<u8> bytes;
	glm::ivec2 size = {};
};
struct Texture::Info : InfoBase
{
	glm::ivec2 size = {};
	Space mode;
	Type type;
	Sampler sampler;
};
struct Texture::CreateInfo
{
	std::vector<stdfs::path> ids;
	std::vector<bytearray> bytes;
	std::vector<Raw> raws;
	stdfs::path samplerID;
	Space mode = Space::eSRGBNonLinear;
	Type type = Type::e2D;
};

struct Material::Inst
{
	Material material;
	res::Texture diffuse;
	res::Texture specular;
	Colour tint = colours::white;
	Colour dropColour = colours::black;
	Flags flags;
};
struct Material::Info : InfoBase
{
	gfx::Albedo albedo;
	f32 shininess = 0.0f;
};
struct Material::CreateInfo
{
	gfx::Albedo albedo;
	f32 shininess = 32.0f;
};

enum class Mesh::Type : s8
{
	eStatic,
	eDynamic,
	eCOUNT_
};
struct Mesh::Info : InfoBase
{
	Material::Inst material;
	Type type;
	u64 triCount = 0;
};
struct Mesh::CreateInfo
{
	gfx::Geometry geometry;
	Material::Inst material;
	Type type = Type::eStatic;
};

struct Font::Glyph
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
struct Font::Info : InfoBase
{
	Material::Inst material;
	Texture sheet;
};
struct Font::CreateInfo
{
	res::Material::Inst material;
	stdfs::path sheetID;
	stdfs::path samplerID;
	stdfs::path materialID;
	std::vector<Glyph> glyphs;
	bytearray image;

	bool deserialise(GData const& json);
};
struct Font::Text
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
} // namespace le::res
