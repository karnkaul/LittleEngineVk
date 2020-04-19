#include <stb/stb_image.h>
#include "core/log.hpp"
#include "core/io.hpp"
#include "core/utils.hpp"
#include "gfx/info.hpp"
#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "resources.hpp"
#include "texture.hpp"

namespace le::gfx
{
Sampler::Sampler(stdfs::path id, Info info) : Resource(std::move(id))
{
	m_createInfo = std::move(info.createInfo);
	m_sampler = info.sampler;
	if (m_sampler == vk::Sampler())
	{
		m_sampler = g_info.device.createSampler(m_createInfo);
	}
	m_status = Status::eReady;
}

Sampler::~Sampler()
{
	vkDestroy(m_sampler);
}

Resource::Status Sampler::update()
{
	return m_status;
}

std::string const Texture::s_tName = utils::tName<Texture>();

Texture::Texture(stdfs::path id, Info info) : Resource(std::move(id)), m_pSampler(info.pSampler)
{
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
		m_raw = std::move(info.raw);
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
	m_loaded = load(&m_active);
	m_imageView = createImageView(m_active.image, vk::Format::eR8G8B8A8Srgb);
#if defined(LEVK_RESOURCE_HOT_RELOAD)
	if (bAddFileMonitor)
	{
		m_pReader = dynamic_cast<FileReader const*>(info.pReader);
		ASSERT(m_pReader, "FileReader required!");
		m_imgID = info.imgID;
		m_files.push_back(File(m_imgID.assetID, m_pReader->fullPath(m_imgID.assetID), FileMonitor::Mode::eBinaryContents, {}));
	}
#endif
}

Texture::~Texture()
{
	if (m_bStbiRaw)
	{
		stbi_image_free((void*)(m_raw.bytes.pData));
		m_raw = {};
	}
	vram::release(m_active);
#if defined(LEVK_RESOURCE_HOT_RELOAD)
	if (m_standby.image != m_active.image)
	{
		vram::release(m_standby);
	}
#endif
	vkDestroy(m_imageView);
}

Resource::Status Texture::update()
{
	if (m_status == Status::eLoading)
	{
		if (isSignalled(m_loaded))
		{
			m_status = Status::eReady;
#if defined(LEVK_RESOURCE_HOT_RELOAD)
			if (m_bReloading)
			{
				m_status = Status::eReloaded;
				m_bReloading = false;
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
#if defined(LEVK_RESOURCE_HOT_RELOAD)
	if (!m_files.empty())
	{
		if (m_status == Status::eReloaded)
		{
			g_info.device.waitIdle();
			vkDestroy(m_imageView);
			if (m_standby.image != vk::Image())
			{
				if (m_active.image != m_standby.image)
				{
					vram::release(m_active);
				}
				m_active = m_standby;
			}
			m_imageView = createImageView(m_active.image, vk::Format::eR8G8B8A8Srgb);
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
				if (m_bStbiRaw)
				{
					stbi_image_free((void*)(m_raw.bytes.pData));
					m_raw = {};
				}
				auto img = Img{file.monitor.bytes(), m_imgID.channels};
				if (!imgToRaw(std::move(img)))
				{
					LOG_E("[{}] [{}] Failed to reload!", s_tName, m_id.generic_string());
				}
				else
				{
					m_loaded = load(&m_standby);
					m_bReloading = true;
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
	m_raw.size = {width, height};
	m_raw.bytes = ArrayView(pOut, size);
	m_bStbiRaw = true;
	return true;
}

vk::Fence Texture::load(Image* out_pImage)
{
	if (out_pImage->image == vk::Image() || out_pImage->extent.width != (u32)m_raw.size.x || out_pImage->extent.height != (u32)m_raw.size.y)
	{
		gfx::ImageInfo imageInfo;
		imageInfo.queueFlags = gfx::QFlag::eTransfer | gfx::QFlag::eGraphics;
		imageInfo.createInfo.format = vk::Format::eR8G8B8A8Srgb;
		imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.createInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		imageInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		imageInfo.createInfo.extent = vk::Extent3D((u32)m_raw.size.x, (u32)m_raw.size.y, 1);
		imageInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.createInfo.imageType = vk::ImageType::e2D;
		imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.createInfo.mipLevels = 1;
		imageInfo.createInfo.arrayLayers = 1;
#if defined(LEVK_VKRESOURCE_NAMES)
		imageInfo.name = m_id.generic_string();
#endif
		*out_pImage = vram::createImage(imageInfo);
	}
	auto ret = vram::copy(m_raw.bytes, *out_pImage, {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal});
	m_status = Status::eLoading;
	return ret;
}
} // namespace le::gfx