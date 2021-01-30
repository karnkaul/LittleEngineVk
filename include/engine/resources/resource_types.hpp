#pragma once
#include <variant>
#include <vector>
#include <core/colour.hpp>
#include <core/hash.hpp>
#include <core/span.hpp>
#include <core/token_gen.hpp>
#include <core/utils.hpp>
#include <core/zero.hpp>
#include <dumb_json/djson.hpp>
#include <engine/gfx/geometry.hpp>
#include <glm/vec2.hpp>
#include <kt/enum_flags/enum_flags.hpp>
#include <kt/result/result.hpp>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_RESOURCES_HOT_RELOAD)
///
/// \brief Enable resource reloading on file changes (requires FileReader)
///
#define LEVK_RESOURCES_HOT_RELOAD
#endif
#endif

namespace le {
#if defined(LEVK_RESOURCES_HOT_RELOAD)
inline constexpr bool levk_resourcesHotReload = true;
#else
inline constexpr bool levk_resourcesHotReload = false;
#endif

namespace res {
using GUID = TZero<u64>;

///
/// \brief Resource status
///
enum class Status : s8 {
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
template <typename T>
struct Resource {
	///
	/// \brief Unique ID per Resource; this is the only data member in all derived types
	///
	GUID guid;

	inline static std::string const s_tName = utils::tName<T>();
};

///
/// \brief Handle for Vulkan shader program
///
/// GLSL to SPIR-V compilation is supported if LEVK_SHADER_COMPILER is defined
///
struct Shader final : Resource<Shader> {
	enum class Type : s8;
	struct Info;
	struct CreateInfo;

	struct Impl;

	Info const& info() const;
	Status status() const;
};

///
/// \brief Handle for Vulkan Image Sampler
///
struct Sampler final : Resource<Sampler> {
	enum class Filter : s8;
	enum class Mode : s8;
	struct Info;
	struct CreateInfo;

	struct Impl;

	Info const& info() const;
	Status status() const;
};

///
/// \brief Handle for Vulkan Image and ImageView
///
struct Texture final : Resource<Texture> {
	enum class Space : s8;
	enum class Type : s8;
	struct Raw;
	struct Info;
	struct CreateInfo;
	struct LoadInfo;

	struct Impl;

	Info const& info() const;
	Status status() const;
};

///
/// \brief Handle for base material used in Mesh
///
struct Material final : Resource<Material> {
	enum class Flag : s8 { eTextured, eLit, eOpaque, eDropColour, eUI, eSkybox, eCOUNT_ };
	using Flags = kt::enum_flags<Flag>;

	struct Inst;
	struct Info;
	struct CreateInfo;

	struct Impl;

	Info const& info() const;
	Status status() const;
};

///
/// \brief Handle for drawable resource using Geometry
///
struct Mesh final : Resource<Mesh> {
	enum class Type : s8;
	struct Info;
	struct CreateInfo;

	struct Impl;

	Info const& info() const;
	Status status() const;

	void updateGeometry(gfx::Geometry geometry);
	Material::Inst& material();
};

///
/// \brief Handle for bitmap font using texture atlas
///
struct Font final : Resource<Font> {
	struct Glyph;
	struct Info;
	struct CreateInfo;
	struct Text;
	using Size = std::variant<u32, f32>;

	struct Layout {
		glm::ivec2 maxBounds = {};
		u32 lineCount = 0;
		f32 lineHeight = 0.0f;
		f32 textHeight = 0.0f;
		f32 scale = 1.0f;
		f32 linePad = 0.2f;
	};

	struct Impl;

	Info const& info() const;
	Status status() const;

	gfx::Geometry generate(Text const& text, std::optional<Layout> layout = std::nullopt) const;
	glm::ivec2 glyphBounds(std::string_view text = {}) const;
	Layout layout(std::string_view text, Size size = 1.0f, f32 nPadY = 0.1f) const;
};

///
/// \brief Handle for model described as a number of meshes, materials, and textures
///
struct Model final : Resource<Model> {
	struct TexData;
	struct MatData;
	struct MeshData;
	struct Info;
	struct CreateInfo;
	struct LoadInfo;

	class Impl;

	Info const& info() const;
	Status status() const;

	std::vector<Mesh> meshes() const;
};

struct InfoBase {
	io::Path id;
};

template <typename T>
struct LoadBase {
	using CreateInfo = typename T::CreateInfo;

	kt::result_t<CreateInfo> createInfo() const;
};

enum class Shader::Type : s8 { eVertex, eFragment, eCOUNT_ };
struct Shader::Info : InfoBase {};
struct Shader::CreateInfo {
	EnumArray<Type, bytearray> codeMap;
	EnumArray<Type, io::Path> codeIDMap;
};

enum class Sampler::Filter : s8 { eLinear, eNearest, eCOUNT_ };
enum class Sampler::Mode : s8 { eRepeat, eClampEdge, eClampBorder, eCOUNT_ };
struct Sampler::Info : InfoBase {
	Filter min = Filter::eLinear;
	Filter mag = Filter::eLinear;
	Filter mip = Filter::eLinear;
	Mode mode = Mode::eRepeat;
};
struct Sampler::CreateInfo {
	Filter min = Filter::eLinear;
	Filter mag = Filter::eLinear;
	Filter mip = Filter::eLinear;
	Mode mode = Mode::eRepeat;
};

enum class Texture::Space : s8 { eSRGBNonLinear, eRGBLinear, eCOUNT_ };
enum class Texture::Type : s8 { e2D, eCube, eCOUNT_ };
struct Texture::Raw {
	Span<u8> bytes;
	glm::ivec2 size = {};
};
struct Texture::Info : InfoBase {
	glm::ivec2 size = {};
	Space mode;
	Type type;
	Sampler sampler;
};
struct Texture::CreateInfo {
	std::vector<io::Path> ids;
	std::vector<bytearray> bytes;
	std::vector<Raw> raws;
	Hash samplerID;
	Space mode = Space::eSRGBNonLinear;
	Type type = Type::e2D;
};
struct Texture::LoadInfo : LoadBase<Texture> {
	io::Path directory;
	std::string imageFilename;
	std::vector<std::string> cubemapFilenames;
	Hash samplerID;
};

struct Material::Inst {
	Material material;
	res::Texture diffuse;
	res::Texture specular;
	Colour tint = colours::white;
	Colour dropColour = colours::black;
	Flags flags;
};
struct Material::Info : InfoBase {
	gfx::Albedo albedo;
	f32 shininess = 0.0f;
};
struct Material::CreateInfo {
	gfx::Albedo albedo;
	f32 shininess = 32.0f;
};

enum class Mesh::Type : s8 { eStatic, eDynamic, eCOUNT_ };
struct Mesh::Info : InfoBase {
	Material::Inst material;
	Type type;
	u64 triCount = 0;
};
struct Mesh::CreateInfo {
	gfx::Geometry geometry;
	Material::Inst material;
	Type type = Type::eStatic;
};

struct Font::Glyph {
	u8 ch = '\0';
	glm::ivec2 st = glm::ivec2(0);
	glm::ivec2 uv = glm::ivec2(0);
	glm::ivec2 cell = glm::ivec2(0);
	glm::ivec2 offset = glm::ivec2(0);
	s32 xAdv = 0;
	s32 orgSizePt = 0;
	bool bBlank = false;

	void deserialise(u8 c, dj::node_t const& json);
};
struct Font::Info : InfoBase {
	Material::Inst material;
	Texture sheet;
	io::Path jsonID;
	glm::ivec2 maxBounds = {};
};
struct Font::CreateInfo {
	res::Material::Inst material;
	io::Path sheetID;
	io::Path jsonID;
	std::vector<Glyph> glyphs;
	bytearray image;
	Hash samplerID;
	Hash materialID;

	bool deserialise(std::string const& jsonStr);
};
struct Font::Text {
	enum class HAlign : s8 { Centre = 0, Left, Right };
	enum class VAlign : s8 { Middle = 0, Top, Bottom };

	std::string text;
	glm::vec3 pos = glm::vec3(0.0f);
	Font::Size size = 1.0f;
	f32 nYPad = 0.2f;
	HAlign halign = HAlign::Centre;
	VAlign valign = VAlign::Middle;
	Colour colour = colours::white;
};

struct Model::TexData {
	io::Path id;
	io::Path filename;
	bytearray bytes;
	Hash samplerID;
	Hash hash;
};
struct Model::MatData {
	io::Path id;
	std::vector<std::size_t> diffuseIndices;
	std::vector<std::size_t> specularIndices;
	std::vector<std::size_t> bumpIndices;
	gfx::Albedo albedo;
	f32 shininess = 32.0f;
	res::Material::Flags flags;
	Hash hash;
};
struct Model::MeshData {
	gfx::Geometry geometry;
	io::Path id;
	std::vector<std::size_t> materialIndices;
	f32 shininess = 32.0f;
	Hash hash;
};
struct Model::Info : InfoBase {
	glm::vec3 origin;
	res::Mesh::Type type;
	res::Texture::Space mode;
	Colour tint;
};
struct Model::CreateInfo {
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
struct Model::LoadInfo : LoadBase<Model> {
	io::Path idRoot;
	io::Path jsonDirectory;
	std::string jsonFilename;
};

template <>
kt::result_t<Texture::CreateInfo> LoadBase<Texture>::createInfo() const;

template <>
kt::result_t<Model::CreateInfo> LoadBase<Model>::createInfo() const;
} // namespace res
} // namespace le
