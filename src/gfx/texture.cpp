#include <array>
#include <stb/stb_image.h>
#include <core/log.hpp>
#include <core/reader.hpp>
#include <core/utils.hpp>
#include <engine/assets/resources.hpp>
#include <engine/gfx/texture.hpp>
#include <engine/resources/resources.hpp>
#include <gfx/common.hpp>
#include <gfx/deferred.hpp>
#include <gfx/device.hpp>
#include <gfx/vram.hpp>
#include <resources/resources_impl.hpp>

namespace le::gfx
{
namespace
{
std::array const g_texModes = {vk::Format::eR8G8B8A8Srgb, vk::Format::eR8G8B8A8Snorm};
std::array const g_texTypes = {vk::ImageViewType::e2D, vk::ImageViewType::eCube};

std::future<void> load(Image* out_pImage, vk::Format texMode, glm::ivec2 const& size, Span<Span<u8>> bytes, [[maybe_unused]] std::string const& name)
{
	if (out_pImage->image == vk::Image() || out_pImage->extent.width != (u32)size.x || out_pImage->extent.height != (u32)size.y)
	{
		ImageInfo imageInfo;
		imageInfo.queueFlags = QFlag::eTransfer | QFlag::eGraphics;
		imageInfo.createInfo.format = texMode;
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

TResult<Texture::Raw> imgToRaw(bytearray imgBytes, std::string_view tName, std::string_view id, io::Level errLevel)
{
	Texture::Raw ret;
	s32 ch;
	auto pIn = reinterpret_cast<stbi_uc const*>(imgBytes.data());
	auto pOut = stbi_load_from_memory(pIn, (s32)imgBytes.size(), &ret.size.x, &ret.size.y, &ch, 4);
	if (!pOut)
	{
		LOG(errLevel, "[{}] [{}] Failed to load image data!", tName, id);
		return {};
	}
	std::size_t const size = (std::size_t)(ret.size.x * ret.size.y * 4);
	ret.bytes = Span(pOut, size);
	return ret;
}
} // namespace

Texture::Texture(stdfs::path id, Info info) : Asset(std::move(id)), m_sampler(info.sampler)
{
	m_colourSpace = info.mode;
	m_uImpl = std::make_unique<TextureImpl>();
	m_uImpl->type = g_texTypes.at((std::size_t)info.type);
	m_uImpl->colourSpace = g_texModes.at((std::size_t)m_colourSpace);
	m_tName = utils::tName<Texture>();
	auto const idStr = m_id.generic_string();
	[[maybe_unused]] bool bAddFileMonitor = false;
	if (m_sampler.guid == 0 || resources::status(m_sampler) != resources::Status::eReady)
	{
		auto const id = info.samplerID.empty() ? "samplers/default" : info.samplerID.generic_string();
		auto [sampler, bResult] = resources::find<resources::Sampler>(id);
		if (!bResult)
		{
			LOG_E("[{}] [{}] Failed to find Sampler [{}]!", m_tName, idStr, info.samplerID.generic_string());
			m_status = Status::eError;
			return;
		}
		m_sampler = sampler;
	}
	if (!info.raws.empty())
	{
		m_uImpl->bStbiRaw = false;
		for (auto& raw : info.raws)
		{
			m_uImpl->raws.push_back(std::move(raw));
		}
	}
	else if (!info.bytes.empty())
	{
		for (auto& bytes : info.bytes)
		{
			auto [raw, bResult] = imgToRaw(std::move(bytes), m_tName, idStr, io::Level::eError);
			if (!bResult)
			{
				LOG_E("[{}] [{}] Failed to create texture!", m_tName, idStr);
				m_status = Status::eError;
				return;
			}
			m_uImpl->raws.push_back(std::move(raw));
		}
		m_uImpl->bStbiRaw = true;
	}
	else if (!info.ids.empty())
	{
		ASSERT(info.pReader, "Reader is null!");
		for (auto const& assetID : info.ids)
		{
			auto [pixels, bPixels] = info.pReader->getBytes(assetID);
			if (!bPixels)
			{
				LOG_E("[{}] [{}] Failed to create texture from [{}]!", m_tName, idStr, assetID.generic_string());
				m_status = Status::eError;
				return;
			}
			auto [raw, bResult] = imgToRaw(std::move(pixels), m_tName, idStr, io::Level::eError);
			if (!bResult)
			{
				LOG_E("[{}] [{}] Failed to create texture from [{}]!", m_tName, idStr, assetID.generic_string());
				m_status = Status::eError;
				return;
			}
			m_uImpl->raws.push_back(std::move(raw));
		}
		m_uImpl->bStbiRaw = true;
		bAddFileMonitor = true;
	}
	else
	{
		ASSERT(false, "Invalid Info!");
		LOG_E("[{}] [{}] Invalid Texture Info!", m_tName, idStr);
		m_status = Status::eError;
		return;
	}
	m_size = m_uImpl->raws.back().size;
	std::vector<Span<u8>> views;
	for (auto const& raw : m_uImpl->raws)
	{
		views.push_back(raw.bytes);
	}
	m_uImpl->copied = load(&m_uImpl->active, m_uImpl->colourSpace, m_size, views, idStr);
	ImageViewInfo viewInfo;
	viewInfo.image = m_uImpl->active.image;
	viewInfo.format = g_texModes.at((std::size_t)m_colourSpace);
	viewInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
	viewInfo.type = m_uImpl->type;
	m_uImpl->imageView = g_device.createImageView(viewInfo);
	m_uImpl->sampler = resources::impl(m_sampler)->sampler;
	m_type = info.type;
	m_status = Status::eLoading;
#if defined(LEVK_ASSET_HOT_RELOAD)
	m_reloadDelay = 50ms;
	if (bAddFileMonitor)
	{
		m_uImpl->pReader = dynamic_cast<io::FileReader const*>(info.pReader);
		ASSERT(m_uImpl->pReader, "io::FileReader required!");
		std::size_t idx = 0;
		for (auto const& id : info.ids)
		{
			m_uImpl->imgIDs.push_back(id);
			auto onModified = [this, idx](File const* pFile) -> bool {
				auto const idStr = m_id.generic_string();
				auto [raw, bResult] = imgToRaw(pFile->monitor.bytes(), m_tName, idStr, io::Level::eWarning);
				if (bResult)
				{
					if (m_uImpl->bStbiRaw)
					{
						stbi_image_free((void*)(m_uImpl->raws.at(idx).bytes.pData));
					}
					m_uImpl->raws.at(idx) = std::move(raw);
					return true;
				}
				return false;
			};
			++idx;
			m_files.push_back(File(id, m_uImpl->pReader->fullPath(id), io::FileMonitor::Mode::eBinaryContents, onModified));
		}
	}
#endif
}

Texture::Texture(Texture&&) = default;
Texture& Texture::operator=(Texture&&) = default;

Texture::~Texture()
{
	if (m_uImpl)
	{
		for (auto& raw : m_uImpl->raws)
		{
			if (m_uImpl->bStbiRaw && raw.bytes.pData)
			{
				stbi_image_free((void*)(raw.bytes.pData));
			}
		}
		deferred::release(m_uImpl->active, m_uImpl->imageView);
#if defined(LEVK_ASSET_HOT_RELOAD)
		if (m_uImpl->standby.image != m_uImpl->active.image)
		{
			deferred::release(m_uImpl->standby);
		}
#endif
	}
}

Asset::Status Texture::update()
{
	auto const idStr = m_id.generic_string();
	if (!m_uImpl)
	{
		m_status = Status::eMoved;
		return m_status;
	}
	Asset::update();
	if (m_status == Status::eLoading || m_status == Status::eReloading)
	{
		if (utils::futureState(m_uImpl->copied) == FutureState::eReady)
		{
			if (m_status == Status::eReloading)
			{
				m_status = Status::eReloaded;
				LOG_I("[{}] [{}] reloaded", m_tName, idStr);
			}
			else
			{
				LOG_D("[{}] [{}] loaded", m_tName, idStr);
				m_status = Status::eReady;
			}
		}
		else
		{
			return m_status;
		}
	}
#if defined(LEVK_ASSET_HOT_RELOAD)
	if (m_status == Status::eReloaded)
	{
		deferred::release(m_uImpl->active, m_uImpl->imageView);
		m_uImpl->active = m_uImpl->standby;
		m_uImpl->standby = {};
		ImageViewInfo viewInfo;
		viewInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
		viewInfo.image = m_uImpl->active.image;
		viewInfo.format = m_uImpl->colourSpace;
		viewInfo.type = m_uImpl->type;
		m_uImpl->imageView = g_device.createImageView(viewInfo);
	}
#endif
	return m_status;
}

#if defined(LEVK_ASSET_HOT_RELOAD)
void Texture::onReload()
{
	auto const idStr = m_id.generic_string();
	std::vector<Span<u8>> views;
	for (auto const& raw : m_uImpl->raws)
	{
		views.push_back(raw.bytes);
	}
	m_uImpl->copied = load(&m_uImpl->standby, m_uImpl->colourSpace, m_size, views, idStr);
	LOG_D("[{}] [{}] reloading...", m_tName, idStr);
}
#endif
} // namespace le::gfx
