#pragma once
#include "core/std_types.hpp"
#include "gfx/common.hpp"
#include "resource.hpp"
#if defined(LEVK_ASSET_HOT_RELOAD)
#include "core/io.hpp"
#endif

namespace le::gfx
{
class Sampler final : public Resource
{
public:
	struct Info final
	{
		vk::SamplerCreateInfo createInfo;
		vk::Sampler sampler = {};
	};

public:
	vk::SamplerCreateInfo m_createInfo;
	vk::Sampler m_sampler;

public:
	Sampler(Info const& info);
	~Sampler() override;
};

class Texture final : public Resource
{
public:
	struct Raw final
	{
		ArrayView<u8> bytes;
		glm::ivec2 size = {};
	};
	struct Img final
	{
		bytearray bytes;
		u8 channels = 4;
	};
	struct ImgID final
	{
		stdfs::path id;
		u8 channels = 4;
	};
	struct Info final
	{
		stdfs::path samplerID;
		Img img;
		Raw raw;
		ImgID imgID;
		Sampler* pSampler = nullptr;
		class IOReader const* pReader = nullptr;
	};

public:
	static std::string const s_tName;

public:
	vk::ImageView m_imageView;
	Sampler* m_pSampler;

private:
	Image m_active;
	Raw m_raw;
	vk::Fence m_loaded;
	bool m_bStbiRaw = false;

#if defined(LEVK_ASSET_HOT_RELOAD)
private:
	Image m_standby;
	ImgID m_imgID;
	class FileReader const* m_pReader = nullptr;
	bool m_bReloading = false;
#endif

public:
	Texture(Info const& info);
	~Texture() override;

public:
	Status updateStatus();

public:
	Status update() override;

private:
	TResult<Img> idToImg(stdfs::path const& id, u8 channels, IOReader const* pReader);
	bool imgToRaw(Img img);
	vk::Fence load(Image* out_pImage);
};
} // namespace le::gfx
