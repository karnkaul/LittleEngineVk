#pragma once
#include <array>
#include <string>
#include <glm/glm.hpp>
#include <core/std_types.hpp>
#include <engine/assets/asset.hpp>

namespace le::gfx
{
namespace rd
{
class Set;
}

class Sampler final : public Asset
{
public:
	enum class Filter : s8
	{
		eLinear,
		eNearest,
		eCOUNT_
	};

	enum class Mode : s8
	{
		eRepeat,
		eClampEdge,
		eClampBorder,
		eCOUNT_
	};

	struct Info final
	{
		Filter min = Filter::eLinear;
		Filter mag = Filter::eLinear;
		Filter mip = Filter::eLinear;
		Mode mode = Mode::eRepeat;
	};

public:
	std::unique_ptr<struct SamplerImpl> m_uImpl;

public:
	Sampler(stdfs::path id, Info info);
	~Sampler() override;

public:
	Status update() override;
};

class Texture final : public Asset
{
public:
	enum class Space : s8
	{
		eSRGBNonLinear,
		eRGBLinear,
		eCOUNT_
	};
	enum class Type : s8
	{
		e2D,
		eCube,
		eCOUNT_
	};
	struct Raw final
	{
		Span<u8> bytes;
		glm::ivec2 size = {};
	};
	struct Info final
	{
		std::vector<stdfs::path> ids;
		std::vector<bytearray> bytes;
		std::vector<Raw> raws;
		stdfs::path samplerID;
		Space mode = Space::eSRGBNonLinear;
		Type type = Type::e2D;
		Sampler* pSampler = nullptr;
		class IOReader const* pReader = nullptr;
	};

public:
	glm::ivec2 m_size = {};
	Space m_colourSpace;
	Type m_type;
	Sampler* m_pSampler = nullptr;

public:
	std::unique_ptr<struct TextureImpl> m_uImpl;

public:
	Texture(stdfs::path id, Info info);
	Texture(Texture&&);
	Texture& operator=(Texture&&);
	~Texture() override;

public:
	Status update() override;

#if defined(LEVK_ASSET_HOT_RELOAD)
protected:
	void onReload() override;
#endif
};
} // namespace le::gfx
