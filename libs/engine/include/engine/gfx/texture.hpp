#pragma once
#include <string>
#include <glm/glm.hpp>
#include "core/std_types.hpp"
#include "engine/assets/asset.hpp"

namespace le::gfx
{
class Sampler final : public Asset
{
public:
	enum class Filter : u8
	{
		eLinear,
		eNearest,
		eCOUNT_
	};

	enum class Mode : u8
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
	struct Raw final
	{
		ArrayView<u8> bytes;
		glm::ivec2 size = {};
	};
	struct Info final
	{
		stdfs::path samplerID;
		bytearray imgBytes;
		Raw raw;
		stdfs::path assetID;
		Sampler* pSampler = nullptr;
		class IOReader const* pReader = nullptr;
	};

public:
	static std::string const s_tName;

public:
	glm::ivec2 m_size = {};
	Sampler* m_pSampler;
	std::unique_ptr<struct TextureImpl> m_uImpl;

public:
	Texture(stdfs::path id, Info info);
	~Texture() override;

public:
	Status update() override;

private:
	TResult<bytearray> idToImg(stdfs::path const& id, IOReader const* pReader);
	bool imgToRaw(bytearray img);
};
} // namespace le::gfx
