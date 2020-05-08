#include <array>
#include <stb/stb_image.h>
#include "core/log.hpp"
#include "core/io.hpp"
#include "core/utils.hpp"
#include "engine/assets/resources.hpp"
#include "engine/gfx/texture.hpp"
#include "gfx/common.hpp"
#include "gfx/info.hpp"
#include "gfx/utils.hpp"
#include "gfx/vram.hpp"

namespace le::gfx
{
namespace
{
std::array const g_filters = {vk::Filter::eLinear, vk::Filter::eNearest};
std::array const g_mipModes = {vk::SamplerMipmapMode::eLinear, vk::SamplerMipmapMode::eNearest};
std::array const g_modes = {vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToBorder};

vk::Fence load(Image* out_pImage, glm::ivec2 const& size, ArrayView<ArrayView<u8>> bytes, [[maybe_unused]] std::string const& name)
{
	if (out_pImage->image == vk::Image() || out_pImage->extent.width != (u32)size.x || out_pImage->extent.height != (u32)size.y)
	{
		gfx::ImageInfo imageInfo;
		imageInfo.queueFlags = gfx::QFlag::eTransfer | gfx::QFlag::eGraphics;
		imageInfo.createInfo.format = vk::Format::eR8G8B8A8Srgb;
		imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.createInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		if (bytes.extent > 1)
		{
			imageInfo.createInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
		}
		imageInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		imageInfo.createInfo.extent = vk::Extent3D((u32)size.x, (u32)size.y, 1);
		imageInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.createInfo.imageType = vk::ImageType::e2D;
		imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.createInfo.mipLevels = 1;
		imageInfo.createInfo.arrayLayers = (u32)bytes.extent;
#if defined(LEVK_VKRESOURCE_NAMES)
		imageInfo.name = name;
#endif
		*out_pImage = vram::createImage(imageInfo);
	}
	return vram::copy(bytes, *out_pImage, {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal});
}

TResult<Texture::Raw> imgToRaw(bytearray imgBytes, std::string_view tName, std::string_view id)
{
	Texture::Raw ret;
	s32 ch;
	auto pIn = reinterpret_cast<stbi_uc const*>(imgBytes.data());
	auto pOut = stbi_load_from_memory(pIn, (s32)imgBytes.size(), &ret.size.x, &ret.size.y, &ch, 4);
	if (!pOut)
	{
		LOG_E("[{}] [{}] Failed to load image data!", tName, id);
		return {};
	}
	size_t const size = (size_t)(ret.size.x * ret.size.y * 4);
	ret.bytes = ArrayView(pOut, size);
	return ret;
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
	samplerInfo.mipmapMode = g_mipModes.at((size_t)info.mip);
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
	auto const idStr = m_id.generic_string();
	m_uImpl = std::make_unique<TextureImpl>();
	[[maybe_unused]] bool bAddFileMonitor = false;
	if (!m_pSampler)
	{
		auto const id = info.samplerID.empty() ? "samplers/default" : info.samplerID.generic_string();
		m_pSampler = g_pResources->get<Sampler>(id);
	}
	if (!m_pSampler)
	{
		LOG_E("[{}] [{}] Failed to find Sampler [{}]!", s_tName, idStr, info.samplerID.generic_string());
		m_status = Status::eError;
		return;
	}
	if (info.raw.bytes.extent > 0)
	{
		m_size = info.raw.size;
		m_uImpl->raw = std::move(info.raw);
	}
	else if (!info.imgBytes.empty())
	{
		auto [raw, bResult] = imgToRaw(std::move(info.imgBytes), s_tName, id.generic_string());
		if (!bResult)
		{
			LOG_E("[{}] [{}] Failed to create texture!", s_tName, idStr);
			m_status = Status::eError;
			return;
		}
		m_uImpl->raw = std::move(raw);
		m_uImpl->bStbiRaw = true;
		m_size = m_uImpl->raw.size;
	}
	else if (!info.assetID.empty())
	{
		ASSERT(info.pReader, "Reader is null!");
		auto [pixels, bBytes] = info.pReader->getBytes(info.assetID);
		if (!bBytes)
		{
			LOG_E("[{}] [{}] Failed to create texture from [{}]!", s_tName, idStr, info.assetID.generic_string());
			m_status = Status::eError;
			return;
		}
		auto [raw, bRaw] = imgToRaw(std::move(pixels), s_tName, id.generic_string());
		if (!bRaw)
		{
			LOG_E("[{}] [{}] Failed to create texture from [{}]!", s_tName, idStr, info.assetID.generic_string());
			m_status = Status::eError;
			return;
		}
		m_uImpl->raw = std::move(raw);
		m_uImpl->bStbiRaw = true;
		m_size = m_uImpl->raw.size;
		bAddFileMonitor = true;
	}
	else
	{
		LOG_E("[{}] [{}] Invalid Info!", s_tName, idStr);
		m_status = Status::eError;
		return;
	}
	m_uImpl->loaded = load(&m_uImpl->active, m_uImpl->raw.size, {m_uImpl->raw.bytes}, idStr);
	m_uImpl->imageView = createImageView(m_uImpl->active.image, vk::Format::eR8G8B8A8Srgb);
#if defined(LEVK_ASSET_HOT_RELOAD)
	if (bAddFileMonitor)
	{
		m_uImpl->pReader = dynamic_cast<FileReader const*>(info.pReader);
		ASSERT(m_uImpl->pReader, "FileReader required!");
		m_uImpl->imgID = info.assetID;
		m_files.push_back(File(m_uImpl->imgID, m_uImpl->pReader->fullPath(m_uImpl->imgID), FileMonitor::Mode::eBinaryContents, {}));
	}
#endif
}

Texture::Texture(Texture&&) = default;
Texture& Texture::operator=(Texture&&) = default;

Texture::~Texture()
{
	if (m_uImpl)
	{
		if (m_uImpl->bStbiRaw)
		{
			stbi_image_free((void*)(m_uImpl->raw.bytes.pData));
			m_uImpl->raw = {};
		}
		waitFor(m_uImpl->loaded);
		vram::release(m_uImpl->active);
#if defined(LEVK_ASSET_HOT_RELOAD)
		if (m_uImpl->standby.image != m_uImpl->active.image)
		{
			vram::release(m_uImpl->standby);
		}
#endif
		vkDestroy(m_uImpl->imageView);
	}
}

Asset::Status Texture::update()
{
	if (!m_uImpl)
	{
		m_status = Status::eMoved;
		return m_status;
	}
	auto const idStr = m_id.generic_string();
	if (m_status == Status::eLoading)
	{
		if (isSignalled(m_uImpl->loaded))
		{
			m_uImpl->loaded = vk::Fence();
			m_status = Status::eReady;
#if defined(LEVK_ASSET_HOT_RELOAD)
			if (m_uImpl->bReloading)
			{
				m_status = Status::eReloaded;
				m_uImpl->bReloading = false;
			}
#endif
			LOG_D("[{}] [{}] loaded", s_tName, idStr);
		}
		else
		{
			m_status = Status::eLoading;
			LOG_D("[{}] loading...", idStr);
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
				LOG_W("[{}] [{}] Resource not ready / lost!", s_tName, idStr);
			}
			else if (lastStatus == FileMonitor::Status::eModified && currentStatus == FileMonitor::Status::eUpToDate)
			{
				if (m_uImpl->bStbiRaw)
				{
					stbi_image_free((void*)(m_uImpl->raw.bytes.pData));
					m_uImpl->raw = {};
				}
				auto [raw, bResult] = imgToRaw(file.monitor.bytes(), s_tName, idStr);
				if (!bResult)
				{
					LOG_E("[{}] [{}] Failed to reload!", s_tName, idStr);
				}
				else
				{
					m_uImpl->raw = std::move(raw);
					m_size = m_uImpl->raw.size;
					m_uImpl->loaded = load(&m_uImpl->standby, m_uImpl->raw.size, {m_uImpl->raw.bytes}, idStr);
					m_status = Status::eLoading;
					m_uImpl->bReloading = true;
					LOG_D("[{}] [{}] reloading...", s_tName, idStr);
				}
			}
		}
	}
#endif
	return m_status;
}

std::string const Cubemap::s_tName = utils::tName<Cubemap>();

Cubemap::Cubemap(stdfs::path id, Info info) : Asset(std::move(id)), m_pSampler(info.pSampler)
{
	m_uImpl = std::make_unique<CubemapImpl>();
	auto const idStr = m_id.generic_string();
	[[maybe_unused]] bool bAddFileMonitor = false;
	if (!m_pSampler)
	{
		auto const id = info.samplerID.empty() ? "samplers/default" : info.samplerID.generic_string();
		m_pSampler = g_pResources->get<Sampler>(id);
	}
	if (!m_pSampler)
	{
		LOG_E("[{}] [{}] Failed to find Sampler [{}]!", s_tName, idStr, info.samplerID.generic_string());
		m_status = Status::eError;
		return;
	}
	glm::ivec2 size = {};
	if (!info.rludfb.at(0).empty())
	{
		size_t idx = 0;
		for (auto& bytes : info.rludfb)
		{
			auto [raw, bRaw] = imgToRaw(std::move(bytes), s_tName, idStr);
			if (!bRaw)
			{
				LOG_E("[{}] [{}] Failed to create texture!", s_tName, idStr);
				m_status = Status::eError;
				return;
			}
			size += raw.size;
			m_uImpl->rludfb.at(idx++) = std::move(raw);
		}
	}
	else
	{
		ASSERT(info.pReader, "Reader is null!");
		size_t idx = 0;
		for (auto assetID : info.rludfbIDs)
		{
			auto [pixels, bPixels] = info.pReader->getBytes(assetID);
			if (!bPixels)
			{
				LOG_E("[{}] [{}] Failed to create texture from [{}]!", s_tName, idStr, assetID.generic_string());
				m_status = Status::eError;
				return;
			}
			auto [raw, bRaw] = imgToRaw(std::move(pixels), s_tName, idStr);
			if (!bRaw)
			{
				LOG_E("[{}] [{}] Failed to create texture from [{}]!", s_tName, idStr, assetID.generic_string());
				m_status = Status::eError;
				return;
			}
			size += raw.size;
			m_uImpl->rludfb.at(idx++) = std::move(raw);
		}
		bAddFileMonitor = true;
	}
	std::array<ArrayView<u8>, 6> rludfb;
	size_t idx = 0;
	for (auto const& raw : m_uImpl->rludfb)
	{
		rludfb.at(idx++) = raw.bytes;
	}
	m_uImpl->loaded = load(&m_uImpl->active, size, rludfb, idStr);
	m_uImpl->imageView = createImageView(m_uImpl->active.image, vk::Format::eR8G8B8A8Srgb);
#if defined(LEVK_ASSET_HOT_RELOAD)
	if (bAddFileMonitor)
	{
		m_uImpl->pReader = dynamic_cast<FileReader const*>(info.pReader);
		ASSERT(m_uImpl->pReader, "FileReader required!");
		m_uImpl->imgIDs = std::move(info.rludfbIDs);
		for (auto const& id : m_uImpl->imgIDs)
		{
			m_files.push_back(File(id, m_uImpl->pReader->fullPath(id), FileMonitor::Mode::eBinaryContents, {}));
		}
	}
#endif
}

Cubemap::Cubemap(Cubemap&&) = default;
Cubemap& Cubemap::operator=(Cubemap&&) = default;

Cubemap::~Cubemap()
{
	if (m_uImpl)
	{
		for (auto const& raw : m_uImpl->rludfb)
		{
			stbi_image_free((void*)(raw.bytes.pData));
		}
		waitFor(m_uImpl->loaded);
		vram::release(m_uImpl->active);
#if defined(LEVK_ASSET_HOT_RELOAD)
		if (m_uImpl->standby.image != m_uImpl->active.image)
		{
			vram::release(m_uImpl->standby);
		}
#endif
		vkDestroy(m_uImpl->imageView);
	}
}

Asset::Status Cubemap::update()
{
	if (!m_uImpl)
	{
		m_status = Status::eMoved;
		return m_status;
	}
	if (m_status == Status::eLoading)
	{
		if (isSignalled(m_uImpl->loaded))
		{
			m_uImpl->loaded = vk::Fence();
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
			LOGIF_E(m_status != Status::eReady, "NOT IMPLEMENTED");
		}
	}
#endif
	return m_status;
}
} // namespace le::gfx
