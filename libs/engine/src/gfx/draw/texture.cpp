#include <array>
#include <stb/stb_image.h>
#include "core/log.hpp"
#include "core/io.hpp"
#include "core/utils.hpp"
#include "engine/assets/resources.hpp"
#include "engine/gfx/draw/texture.hpp"
#include "gfx/common.hpp"
#include "gfx/info.hpp"
#include "gfx/utils.hpp"
#include "gfx/vram.hpp"

namespace le::gfx
{
namespace
{
std::array const g_filters = {vk::Filter::eLinear, vk::Filter::eNearest};
std::array const g_modes = {vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToBorder};

vk::Fence load(Image* out_pImage, glm::ivec2 const& size, ArrayView<u8> bytes, [[maybe_unused]] std::string const& name)
{
	if (out_pImage->image == vk::Image() || out_pImage->extent.width != (u32)size.x || out_pImage->extent.height != (u32)size.y)
	{
		gfx::ImageInfo imageInfo;
		imageInfo.queueFlags = gfx::QFlag::eTransfer | gfx::QFlag::eGraphics;
		imageInfo.createInfo.format = vk::Format::eR8G8B8A8Srgb;
		imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.createInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		imageInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		imageInfo.createInfo.extent = vk::Extent3D((u32)size.x, (u32)size.y, 1);
		imageInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.createInfo.imageType = vk::ImageType::e2D;
		imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.createInfo.mipLevels = 1;
		imageInfo.createInfo.arrayLayers = 1;
#if defined(LEVK_VKRESOURCE_NAMES)
		imageInfo.name = name;
#endif
		*out_pImage = vram::createImage(imageInfo);
	}
	return vram::copy(bytes, *out_pImage, {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal});
}
} // namespace

Sampler::Sampler(stdfs::path id, Info info) : Asset(std::move(id))
{
	m_uImpl = std::make_unique<SamplerImpl>();
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = g_filters.at((size_t)info.min);
	samplerInfo.minFilter = g_filters.at((size_t)info.mag);
	samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = g_modes.at((size_t)info.mode);
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = false;
	samplerInfo.compareEnable = false;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	m_uImpl->sampler = g_info.device.createSampler(samplerInfo);
	m_status = Status::eReady;
}

Sampler::~Sampler()
{
	vkDestroy(m_uImpl->sampler);
}

Asset::Status Sampler::update()
{
	return m_status;
}

std::string const Texture::s_tName = utils::tName<Texture>();

Texture::Texture(stdfs::path id, Info info) : Asset(std::move(id)), m_pSampler(info.pSampler)
{
	m_uImpl = std::make_unique<TextureImpl>();
	[[maybe_unused]] bool bAddFileMonitor = false;
	if (!m_pSampler)
	{
		auto const id = info.samplerID.empty() ? "samplers/default" : info.samplerID.generic_string();
		m_pSampler = g_pResources->get<Sampler>(id);
	}
	if (!m_pSampler)
	{
		throw std::runtime_error("Failed to find sampler");
	}
	if (info.raw.bytes.extent > 0)
	{
		m_uImpl->raw = std::move(info.raw);
	}
	else if (!info.img.bytes.empty())
	{
		if (!imgToRaw(std::move(info.img)))
		{
			throw std::runtime_error("Failed to create texture");
		}
	}
	else if (!info.imgID.assetID.empty())
	{
		auto [pixels, bResult] = info.pReader->getBytes(info.imgID.assetID);
		if (!bResult || !imgToRaw({std::move(pixels), info.imgID.channels}))
		{
			throw std::runtime_error("Failed to create texture");
		}
		bAddFileMonitor = true;
	}
	else
	{
		throw std::runtime_error("Invalid Info");
	}
	m_uImpl->loaded = load(&m_uImpl->active, m_uImpl->raw.size, m_uImpl->raw.bytes, m_id.generic_string());
	m_uImpl->imageView = createImageView(m_uImpl->active.image, vk::Format::eR8G8B8A8Srgb);
#if defined(LEVK_ASSET_HOT_RELOAD)
	if (bAddFileMonitor)
	{
		m_uImpl->pReader = dynamic_cast<FileReader const*>(info.pReader);
		ASSERT(m_uImpl->pReader, "FileReader required!");
		m_uImpl->imgID = info.imgID;
		m_files.push_back(
			File(m_uImpl->imgID.assetID, m_uImpl->pReader->fullPath(m_uImpl->imgID.assetID), FileMonitor::Mode::eBinaryContents, {}));
	}
#endif
}

Texture::~Texture()
{
	if (m_uImpl->bStbiRaw)
	{
		stbi_image_free((void*)(m_uImpl->raw.bytes.pData));
		m_uImpl->raw = {};
	}
	vram::release(m_uImpl->active);
#if defined(LEVK_ASSET_HOT_RELOAD)
	if (m_uImpl->standby.image != m_uImpl->active.image)
	{
		vram::release(m_uImpl->standby);
	}
#endif
	vkDestroy(m_uImpl->imageView);
}

Asset::Status Texture::update()
{
	if (m_status == Status::eLoading)
	{
		if (isSignalled(m_uImpl->loaded))
		{
			m_status = Status::eReady;
#if defined(LEVK_ASSET_HOT_RELOAD)
			if (m_uImpl->bReloading)
			{
				m_status = Status::eReloaded;
				m_uImpl->bReloading = false;
			}
#endif
			LOG_D("[{}] [{}] loaded", s_tName, m_id.generic_string());
		}
		else
		{
			m_status = Status::eLoading;
			LOG_D("[{}] loading...", m_id.generic_string());
		}
	}
	if (m_status == Status::eLoading)
	{
		// Stall Hot Reloading until this file has finished loading to the GPU
		return m_status;
	}
#if defined(LEVK_ASSET_HOT_RELOAD)
	if (!m_files.empty())
	{
		if (m_status == Status::eReloaded)
		{
			g_info.device.waitIdle();
			vkDestroy(m_uImpl->imageView);
			if (m_uImpl->standby.image != vk::Image())
			{
				if (m_uImpl->active.image != m_uImpl->standby.image)
				{
					vram::release(m_uImpl->active);
				}
				m_uImpl->active = m_uImpl->standby;
			}
			m_uImpl->imageView = createImageView(m_uImpl->active.image, vk::Format::eR8G8B8A8Srgb);
			m_status = Status::eReady;
		}
		else
		{
			auto& file = m_files.front();
			auto lastStatus = file.monitor.lastStatus();
			auto currentStatus = file.monitor.update();
			if (currentStatus == FileMonitor::Status::eNotFound)
			{
				LOG_E("[{}] [{}] Resource lost!", s_tName, m_id.generic_string());
			}
			else if (lastStatus == FileMonitor::Status::eModified && currentStatus == FileMonitor::Status::eUpToDate)
			{
				if (m_uImpl->bStbiRaw)
				{
					stbi_image_free((void*)(m_uImpl->raw.bytes.pData));
					m_uImpl->raw = {};
				}
				auto img = Img{file.monitor.bytes(), m_uImpl->imgID.channels};
				if (!imgToRaw(std::move(img)))
				{
					LOG_E("[{}] [{}] Failed to reload!", s_tName, m_id.generic_string());
				}
				else
				{
					m_uImpl->loaded = load(&m_uImpl->standby, m_uImpl->raw.size, m_uImpl->raw.bytes, m_id.generic_string());
					m_status = Status::eLoading;
					m_uImpl->bReloading = true;
					LOG_D("[{}] [{}] reloading...", s_tName, m_id.generic_string());
				}
			}
		}
	}
#endif
	return m_status;
}

TResult<Texture::Img> Texture::idToImg(stdfs::path const& id, u8 channels, IOReader const* pReader)
{
	auto [pixels, bResult] = pReader->getBytes(id);
	if (!bResult)
	{
		LOG_E("[{}] [{}] Failed to find [{}] on [{}]!", s_tName, m_id.generic_string(), id.generic_string(), pReader->medium());
		return {};
	}
	return Img{std::move(pixels), channels};
}

bool Texture::imgToRaw(Img img)
{
	s32 width, height, ch;
	auto pIn = reinterpret_cast<stbi_uc const*>(img.bytes.data());
	auto pOut = stbi_load_from_memory(pIn, (s32)img.bytes.size(), &width, &height, &ch, img.channels);
	if (!pOut)
	{
		LOG_E("[{}] [{}] Failed to load image data!", s_tName, m_id.generic_string());
		return false;
	}
	size_t const size = (size_t)(width * height * img.channels);
	m_uImpl->raw.size = {width, height};
	m_uImpl->raw.bytes = ArrayView(pOut, size);
	m_uImpl->bStbiRaw = true;
	return true;
}
} // namespace le::gfx
