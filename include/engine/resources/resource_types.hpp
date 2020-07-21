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
///
/// \brief Enable resource reloading on file changes (requires FileReader)
///
#define LEVK_RESOURCES_HOT_RELOAD
#endif
#endif

namespace le::res
{
using GUID = TZero<u64>;

///
/// \brief Resource status
///
enum class Status : s8
{
	eIdle,
	eReady,		// ready to use
	eLoading,	// transferring resources
	eReloading, // reloading (only relevant if `LEVK_RESOURCES_HOT_RELOAD` is defined)
	eError,		// cannot be used
	eCOUNT_
};

///
/// \brief Base struct for all resources
///
/// All derived types define the following subtypes:
/// 	Info		: read-only data corresponding to handle
/// 	CreateInfo	: setup data passed to resources::load
/// 	Impl		: implementation detail (internal to engine)
///
struct Resource
{
	///
	/// \brief Unique ID per Resource; this is the only data member in all derived types
	///
	GUID guid;
};

///
/// \brief Handle for Vulkan shader program
///
/// GLSL to SPIR-V compilation is supported if LEVK_SHADER_COMPILER is defined
///
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

///
/// \brief Handle for Vulkan Image Sampler
///
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

///
/// \brief Handle for Vulkan Image and ImageView
///
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

///
/// \brief Handle for base material used in Mesh
///
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

///
/// \brief Handle for drawable resource using Geometry
///
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

///
/// \brief Handle for bitmap font using texture atlas
///
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

///
/// \brief Handle for model described as a number of meshes, materials, and textures
///
struct Model final : Resource
{
	struct TexData;
	struct MatData;
	struct MeshData;
	struct Info;
	struct CreateInfo;

	class Impl;

	static std::string const s_tName;

	static CreateInfo parseOBJ(stdfs::path const& resourceID);

	Info const& info() const;
	Status status() const;

	std::vector<Mesh> meshes() const;
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

struct Model::TexData
{
	stdfs::path id;
	stdfs::path samplerID;
	stdfs::path filename;
	bytearray bytes;
	Hash hash;
};
struct Model::MatData
{
	stdfs::path id;
	std::vector<std::size_t> diffuseIndices;
	std::vector<std::size_t> specularIndices;
	std::vector<std::size_t> bumpIndices;
	gfx::Albedo albedo;
	f32 shininess = 32.0f;
	res::Material::Flags flags;
	Hash hash;
};
struct Model::MeshData
{
	gfx::Geometry geometry;
	stdfs::path id;
	std::vector<std::size_t> materialIndices;
	f32 shininess = 32.0f;
	Hash hash;
};
struct Model::Info : InfoBase
{
	glm::vec3 origin;
	res::Mesh::Type type;
	res::Texture::Space mode;
	Colour tint;
};
struct Model::CreateInfo
{
	glm::vec3 origin = glm::vec3(0.0f);
	std::vector<TexData> textures;
	std::vector<MatData> materials;
	std::vector<MeshData> meshData;
	std::vector<res::Mesh> preloaded;
	res::Mesh::Type type = res::Mesh::Type::eStatic;
	res::Texture::Space mode = res::Texture::Space::eSRGBNonLinear;
	Colour tint = colours::white;
	bool bDropColour = false;
};
} // namespace le::res
