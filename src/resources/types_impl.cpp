#include <stb/stb_image.h>
#include <core/log.hpp>
#include <gfx/common.hpp>
#include <gfx/deferred.hpp>
#include <gfx/device.hpp>
#include <gfx/vram.hpp>
#include <engine/resources/resources.hpp>
#include <engine/resources/shader_compiler.hpp>
#include <resources/resources_impl.hpp>
#include <levk_impl.hpp>

namespace le::resources
{
namespace
{
std::array const g_filters = {vk::Filter::eLinear, vk::Filter::eNearest};
std::array const g_mipModes = {vk::SamplerMipmapMode::eLinear, vk::SamplerMipmapMode::eNearest};
std::array const g_samplerModes = {vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToBorder};
std::array const g_texModes = {vk::Format::eR8G8B8A8Srgb, vk::Format::eR8G8B8A8Snorm};
std::array const g_texTypes = {vk::ImageViewType::e2D, vk::ImageViewType::eCube};

std::future<void> load(gfx::Image& out_image, vk::Format texMode, glm::ivec2 const& size, Span<Span<u8>> bytes, [[maybe_unused]] std::string_view name)
{
	if (out_image.image == vk::Image() || out_image.extent.width != (u32)size.x || out_image.extent.height != (u32)size.y)
	{
		gfx::ImageInfo imageInfo;
		imageInfo.queueFlags = gfx::QFlag::eTransfer | gfx::QFlag::eGraphics;
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
		imageInfo.name = std::string(name);
#endif
		out_image = gfx::vram::createImage(imageInfo);
	}
	return gfx::vram::copy(bytes, out_image, {vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal});
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

std::string const Shader::s_tName = utils::tName<Shader>();
std::string const Sampler::s_tName = utils::tName<Sampler>();
std::string const Texture::s_tName = utils::tName<Texture>();

std::string_view Shader::Impl::s_spvExt = ".spv";
std::string_view Shader::Impl::s_vertExt = ".vert";
std::string_view Shader::Impl::s_fragExt = ".frag";

std::string Shader::Impl::extension(stdfs::path const& id)
{
	auto const str = id.generic_string();
	if (auto idx = str.find_last_of('.'); idx != std::string::npos)
	{
		return str.substr(idx);
	}
	return {};
}

bool Shader::Impl::make(CreateInfo& out_info)
{
	bool const bCodeMapPopulated = std::any_of(out_info.codeMap.begin(), out_info.codeMap.end(), [&](auto const& entry) { return !entry.empty(); });
	[[maybe_unused]] bool const bCodeIDsPopulated =
		std::any_of(out_info.codeIDMap.begin(), out_info.codeIDMap.end(), [&](auto const& entry) { return !entry.empty(); });
	if (!bCodeMapPopulated)
	{
		ASSERT(bCodeIDsPopulated, "Invalid Shader ShaderData!");
		for (std::size_t idx = 0; idx < out_info.codeIDMap.size(); ++idx)
		{
			auto& codeID = out_info.codeIDMap.at(idx);
			auto const ext = extension(codeID);
			bool bSpv = true;
			if (ext == s_vertExt || ext == s_fragExt)
			{
#if defined(LEVK_SHADER_COMPILER)
				if (loadGlsl(codeID, (Type)idx))
				{
#if defined(LEVK_RESOURCES_HOT_RELOAD)
					auto pReader = dynamic_cast<io::FileReader const*>(&engine::reader());
					ASSERT(pReader, "FileReader needed!");
					auto onReloaded = [guid = this->guid, idx](Monitor::File const* pFile) -> bool {
						bool bSuccess = false;
						Shader shader;
						shader.guid = guid;
						if (auto pImpl = resources::impl(shader))
						{
							if (pImpl->glslToSpirV(pFile->id, pImpl->codeMap.at(idx)))
							{
								bSuccess = true;
							}
						}
						if (!bSuccess)
						{
							LOG_E("[{}] Failed to reload Shader!", s_tName);
							return false;
						}
						return true;
					};
					monitor.m_files.push_back({codeID, pReader->fullPath(codeID), io::FileMonitor::Mode::eTextContents, onReloaded});
#endif
				}
				else
				{
					LOG_E("[{}] Failed to compile GLSL code to SPIR-V!", s_tName);
					return false;
				}
				bSpv = false;
#else
				id += s_spvExt;
#endif
			}
			if (bSpv)
			{
				auto [shaderShaderData, bResult] = engine::reader().getBytes(codeID);
				ASSERT(bResult, "Shader code missing!");
				if (!bResult)
				{
					LOG_E("[{}] [{}] Shader code missing: [{}]!", s_tName, id.generic_string(), codeID.generic_string());
					return false;
				}
				out_info.codeMap.at(idx) = std::move(shaderShaderData);
			}
		}
	}
	loadAllSpirV();
	this->status = Status::eReady;
	return true;
}

void Shader::Impl::release()
{
	for (auto const& shader : shaders)
	{
		gfx::g_device.destroy(shader);
	}
}

#if defined(LEVK_RESOURCES_HOT_RELOAD)
bool Shader::Impl::checkReload()
{
	switch (status)
	{
	case Status::eError:
	case Status::eReady:
	{
		if (monitor.update())
		{
			auto const idStr = id.generic_string();
			LOG_D("[{}] [{}] Reloading...", s_tName, idStr);
			loadAllSpirV();
			LOG_I("[{}] [{}] Reloaded", s_tName, idStr);
			return true;
		}
		return false;
	}
	default:
	{
		return false;
	}
	}
}
#endif

#if defined(LEVK_SHADER_COMPILER)
bool Shader::Impl::loadGlsl(stdfs::path const& id, Type type)
{
	if (ShaderCompiler::instance().status() != ShaderCompiler::Status::eOnline)
	{
		LOG_E("[{}] ShaderCompiler is Offline!", s_tName);
		return false;
	}
	auto pReader = dynamic_cast<io::FileReader const*>(&engine::reader());
	ASSERT(pReader, "Cannot compile shaders without io::FileReader!");
	auto [glslCode, bResult] = pReader->getString(id);
	return bResult && glslToSpirV(id, codeMap.at((std::size_t)type));
}

bool Shader::Impl::glslToSpirV(stdfs::path const& id, bytearray& out_bytes)
{
	if (ShaderCompiler::instance().status() != ShaderCompiler::Status::eOnline)
	{
		LOG_E("[{}] ShaderCompiler is Offline!", s_tName);
		return false;
	}
	auto pReader = dynamic_cast<io::FileReader const*>(&engine::reader());
	ASSERT(pReader, "Cannot compile shaders without io::FileReader!");
	auto [glslCode, bResult] = pReader->getString(id);
	if (bResult)
	{
		auto const src = pReader->fullPath(id);
		auto dstID = id;
		dstID += s_spvExt;
		if (!ShaderCompiler::instance().compile(src, true))
		{
			return false;
		}
		auto [spvCode, bResult] = pReader->getBytes(dstID);
		if (!bResult)
		{
			return false;
		}
		out_bytes = std::move(spvCode);
		return true;
	}
	return false;
}
#endif

void Shader::Impl::loadAllSpirV()
{
	for (std::size_t idx = 0; idx < codeMap.size(); ++idx)
	{
		auto const& code = codeMap.at(idx);
		if (!code.empty())
		{
			gfx::g_device.destroy(shaders.at(idx));
			vk::ShaderModuleCreateInfo createInfo;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<u32 const*>(code.data());
			shaders.at(idx) = gfx::g_device.device.createShaderModule(createInfo);
		}
	}
	status = Status::eReady;
}

vk::ShaderModule Shader::Impl::module(Shader::Type type) const
{
	ASSERT(shaders.at((std::size_t)type) != vk::ShaderModule(), "Module not present in Shader!");
	return shaders.at((std::size_t)type);
}

std::map<Shader::Type, vk::ShaderModule> Shader::Impl::modules() const
{
	std::map<Shader::Type, vk::ShaderModule> ret;
	for (std::size_t idx = 0; idx < (std::size_t)Shader::Type::eCOUNT_; ++idx)
	{
		auto const& module = shaders.at(idx);
		if (module != vk::ShaderModule())
		{
			ret[(Shader::Type)idx] = module;
		}
	}
	return ret;
}

Shader::Info const& Shader::info() const
{
	return resources::info(*this);
}

Status Shader::status() const
{
	return resources::status(*this);
}

Sampler::Info const& Sampler::info() const
{
	return resources::info(*this);
}

Status Sampler::status() const
{
	return resources::status(*this);
}

Texture::Info const& Texture::info() const
{
	return resources::info(*this);
}

Status Texture::status() const
{
	return resources::status(*this);
}

bool Sampler::Impl::make(CreateInfo& out_info)
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = g_filters.at((std::size_t)out_info.info.min);
	samplerInfo.minFilter = g_filters.at((std::size_t)out_info.info.mag);
	samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = g_samplerModes.at((std::size_t)out_info.info.mode);
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = false;
	samplerInfo.compareEnable = false;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = g_mipModes.at((std::size_t)out_info.info.mip);
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	sampler = gfx::g_device.device.createSampler(samplerInfo);
	status = Status::eReady;
	return true;
}

void Sampler::Impl::release()
{
	gfx::deferred::release([sampler = this->sampler]() { gfx::g_device.destroy(sampler); });
	sampler = vk::Sampler();
	status = Status::eIdle;
}

bool Texture::Impl::make(CreateInfo& out_info)
{
	auto& info = out_info.info;
	auto const idStr = id.generic_string();
	if (info.sampler.guid == 0 || resources::status(info.sampler) != Status::eReady)
	{
		auto [sampler, bResult] = resources::find<Sampler>("samplers/default");
		if (!bResult)
		{
			LOG_E("[{}] [{}] Failed to locate default sampler", Texture::s_tName, idStr);
			return false;
		}
		info.sampler = sampler;
	}
	if (resources::status(info.sampler) != Status::eReady)
	{
		LOG_E("[{}] [{}] Sampler not ready!", Texture::s_tName, idStr);
		return false;
	}
	type = g_texTypes.at((std::size_t)info.type);
	colourSpace = g_texModes.at((std::size_t)info.mode);
	[[maybe_unused]] bool bAddFileMonitor = false;
	if (!out_info.raws.empty())
	{
		bStbiRaw = false;
		for (auto& raw : out_info.raws)
		{
			raws.push_back(std::move(raw));
		}
	}
	else if (!out_info.bytes.empty())
	{
		for (auto& bytes : out_info.bytes)
		{
			auto [raw, bResult] = imgToRaw(std::move(bytes), Texture::s_tName, idStr, io::Level::eError);
			if (!bResult)
			{
				LOG_E("[{}] [{}] Failed to create texture!", Texture::s_tName, idStr);
				return false;
			}
			raws.push_back(std::move(raw));
		}
		bStbiRaw = true;
	}
	else if (!out_info.ids.empty())
	{
		for (auto const& assetID : out_info.ids)
		{
			auto [pixels, bPixels] = engine::reader().getBytes(assetID);
			if (!bPixels)
			{
				LOG_E("[{}] [{}] Failed to create texture from [{}]!", Texture::s_tName, idStr, assetID.generic_string());
				return false;
			}
			auto [raw, bResult] = imgToRaw(std::move(pixels), Texture::s_tName, idStr, io::Level::eError);
			if (!bResult)
			{
				LOG_E("[{}] [{}] Failed to create texture from [{}]!", Texture::s_tName, idStr, assetID.generic_string());
				return false;
			}
			raws.push_back(std::move(raw));
		}
		bStbiRaw = true;
		bAddFileMonitor = true;
	}
	else
	{
		ASSERT(false, "Invalid Info!");
		LOG_E("[{}] [{}] Invalid Texture Info!", Texture::s_tName, idStr);
		return false;
	}
	info.size = raws.back().size;
	std::vector<Span<u8>> views;
	for (auto const& raw : raws)
	{
		views.push_back(raw.bytes);
	}
	copied = load(active, colourSpace, info.size, views, idStr);
	status = Status::eLoading;
	gfx::ImageViewInfo viewInfo;
	viewInfo.image = active.image;
	viewInfo.format = colourSpace;
	viewInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
	viewInfo.type = type;
	imageView = gfx::g_device.createImageView(viewInfo);
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	monitor = {};
	monitor.m_reloadDelay = 50ms;
	if (bAddFileMonitor)
	{
		auto pReader = dynamic_cast<io::FileReader const*>(&engine::reader());
		ASSERT(pReader, "io::FileReader required!");
		std::size_t idx = 0;
		for (auto const& id : out_info.ids)
		{
			imgIDs.push_back(id);
			auto onModified = [guid = this->guid, idx](Monitor::File const* pFile) -> bool {
				Texture tex;
				tex.guid = guid;
				if (auto pImpl = resources::impl(tex))
				{
					auto const idStr = resources::info(tex).id.generic_string();
					auto [raw, bResult] = imgToRaw(pFile->monitor.bytes(), Texture::s_tName, idStr, io::Level::eWarning);
					if (bResult)
					{
						if (pImpl->bStbiRaw)
						{
							stbi_image_free((void*)(pImpl->raws.at(idx).bytes.pData));
						}
						pImpl->raws.at(idx) = std::move(raw);
						return true;
					}
				}
				return false;
			};
			++idx;
			monitor.m_files.push_back({id, pReader->fullPath(id), io::FileMonitor::Mode::eBinaryContents, onModified});
		}
	}
#endif
	return true;
}

bool Texture::Impl::update()
{
	switch (status)
	{
	case Status::eReady:
	case Status::eError:
	{
		return true;
	}
	case Status::eLoading:
	case Status::eReloading:
	{
		if (utils::futureState(copied) == FutureState::eReady)
		{
			auto const idStr = id.generic_string();
			auto const szLoadStr = status == Status::eReloading ? "reloaded" : "loaded";
			LOG_I("[{}] [{}] {}", Texture::s_tName, idStr, szLoadStr);
#if defined(LEVK_RESOURCES_HOT_RELOAD)
			if (status == Status::eReloading)
			{
				gfx::deferred::release(active, imageView);
				active = standby;
				standby = {};
				gfx::ImageViewInfo viewInfo;
				viewInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
				viewInfo.image = active.image;
				viewInfo.format = colourSpace;
				viewInfo.type = type;
				imageView = gfx::g_device.createImageView(viewInfo);
			}
#endif
			status = Status::eReady;
			return true;
		}
		return false;
	}
	default:
	{
		return true;
	}
	}
}

#if defined(LEVK_RESOURCES_HOT_RELOAD)
bool Texture::Impl::checkReload()
{
	switch (status)
	{
	case Status::eReady:
	{
		if (monitor.update())
		{
			auto const idStr = id.generic_string();
			Texture texture;
			texture.guid = guid;
			auto const& info = resources::info(texture);
			std::vector<Span<u8>> views;
			for (auto const& raw : raws)
			{
				views.push_back(raw.bytes);
			}
			copied = load(standby, colourSpace, info.size, views, idStr);
			status = Status::eReloading;
			LOG_D("[{}] [{}] reloading...", Texture::s_tName, idStr);
			return true;
		}
		return false;
	}
	default:
	{
		return false;
	}
	}
}
#endif

void Texture::Impl::release()
{
	for (auto& raw : raws)
	{
		if (bStbiRaw && raw.bytes.pData)
		{
			stbi_image_free((void*)(raw.bytes.pData));
		}
	}
	gfx::deferred::release(active, imageView);
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	if (standby.image != active.image)
	{
		gfx::deferred::release(standby);
	}
	standby = {};
#endif
	active = {};
	imageView = vk::ImageView();
	status = Status::eIdle;
}
} // namespace le::resources
