#pragma once
#include <vector>
#include <glm/vec2.hpp>
#include <core/hash.hpp>
#include <core/utils.hpp>
#include <core/zero.hpp>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_RESOURCES_HOT_RELOAD)
#define LEVK_RESOURCES_HOT_RELOAD
#endif
#endif

namespace le::resources
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

enum class Shader::Type : s8
{
	eVertex,
	eFragment,
	eCOUNT_
};
struct Shader::Info
{
	stdfs::path id;
};
struct Shader::CreateInfo
{
	Info info;
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
struct Sampler::Info
{
	stdfs::path id;
	Filter min = Filter::eLinear;
	Filter mag = Filter::eLinear;
	Filter mip = Filter::eLinear;
	Mode mode = Mode::eRepeat;
};
struct Sampler::CreateInfo
{
	Info info;
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
struct Texture::Info
{
	stdfs::path id;
	glm::ivec2 size = {};
	Space mode = Space::eSRGBNonLinear;
	Type type = Type::e2D;
	Sampler sampler;
};
struct Texture::CreateInfo
{
	std::vector<stdfs::path> ids;
	std::vector<bytearray> bytes;
	std::vector<Raw> raws;
	stdfs::path samplerID;
	Info info;
};
} // namespace le::resources
