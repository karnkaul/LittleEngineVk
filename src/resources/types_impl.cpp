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

namespace le::res
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

gfx::Buffer createXBO(std::string_view name, vk::DeviceSize size, vk::BufferUsageFlagBits usage, bool bHostVisible)
{
	gfx::BufferInfo bufferInfo;
	bufferInfo.size = size;
	if (bHostVisible)
	{
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	}
	else
	{
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	}
	bufferInfo.usage = usage | vk::BufferUsageFlagBits::eTransferDst;
	bufferInfo.queueFlags = gfx::QFlag::eGraphics | gfx::QFlag::eTransfer;
	bufferInfo.name = name;
	return gfx::vram::createBuffer(bufferInfo);
};
} // namespace

std::string const Shader::s_tName = utils::tName<Shader>();
std::string const Sampler::s_tName = utils::tName<Sampler>();
std::string const Texture::s_tName = utils::tName<Texture>();
std::string const Material::s_tName = utils::tName<Material>();
std::string const Mesh::s_tName = utils::tName<Mesh>();

std::string_view Shader::Impl::s_spvExt = ".spv";
std::string_view Shader::Impl::s_vertExt = ".vert";
std::string_view Shader::Impl::s_fragExt = ".frag";

Shader::Info const& Shader::info() const
{
	return res::info(*this);
}

Sampler::Info const& Sampler::info() const
{
	return res::info(*this);
}

Texture::Info const& Texture::info() const
{
	return res::info(*this);
}

Material::Info const& Material::info() const
{
	return res::info(*this);
}

Mesh::Info const& Mesh::info() const
{
	return res::info(*this);
}

Status Shader::status() const
{
	return res::status(*this);
}

Status Sampler::status() const
{
	return res::status(*this);
}

Status Texture::status() const
{
	return res::status(*this);
}

Status Material::status() const
{
	return res::status(*this);
}

Status Mesh::status() const
{
	return res::status(*this);
}

void Mesh::updateGeometry(gfx::Geometry geometry)
{
	if (auto pImpl = res::impl(*this); auto pInfo = res::infoRW(*this))
	{
		pImpl->updateGeometry(*pInfo, std::move(geometry));
	}
}

Material::Inst& Mesh::material()
{
	static Material::Inst s_default{};
	if (auto pInfo = res::infoRW(*this))
	{
		return pInfo->material;
	}
	return s_default;
}

std::string Shader::Impl::extension(stdfs::path const& id)
{
	auto const str = id.generic_string();
	if (auto idx = str.find_last_of('.'); idx != std::string::npos)
	{
		return str.substr(idx);
	}
	return {};
}

bool Shader::Impl::make(CreateInfo& out_createInfo, Info&)
{
	bool const bCodeMapPopulated = std::any_of(out_createInfo.codeMap.begin(), out_createInfo.codeMap.end(), [&](auto const& entry) { return !entry.empty(); });
	[[maybe_unused]] bool const bCodeIDsPopulated =
		std::any_of(out_createInfo.codeIDMap.begin(), out_createInfo.codeIDMap.end(), [&](auto const& entry) { return !entry.empty(); });
	if (!bCodeMapPopulated)
	{
		ASSERT(bCodeIDsPopulated, "Invalid Shader ShaderData!");
		for (std::size_t idx = 0; idx < out_createInfo.codeIDMap.size(); ++idx)
		{
			auto& codeID = out_createInfo.codeIDMap.at(idx);
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
						if (auto pImpl = res::impl(shader))
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
				out_createInfo.codeMap.at(idx) = std::move(shaderShaderData);
			}
		}
	}
	loadAllSpirV();
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

bool Sampler::Impl::make(CreateInfo& out_createInfo, Info& out_info)
{
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = g_filters.at((std::size_t)out_createInfo.min);
	samplerInfo.minFilter = g_filters.at((std::size_t)out_createInfo.mag);
	samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = g_samplerModes.at((std::size_t)out_createInfo.mode);
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = false;
	samplerInfo.compareEnable = false;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = g_mipModes.at((std::size_t)out_createInfo.mip);
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	sampler = gfx::g_device.device.createSampler(samplerInfo);
	out_info.min = out_createInfo.min;
	out_info.mag = out_createInfo.mag;
	out_info.mip = out_createInfo.mip;
	out_info.mode = out_createInfo.mode;
	return true;
}

void Sampler::Impl::release()
{
	gfx::deferred::release([sampler = this->sampler]() { gfx::g_device.destroy(sampler); });
	sampler = vk::Sampler();
}

bool Texture::Impl::make(CreateInfo& out_createInfo, Info& out_info)
{
	auto const idStr = id.generic_string();
	auto [sampler, bSampler] = res::findSampler(out_createInfo.samplerID);
	if (sampler.guid == res::GUID::s_null || sampler.status() != Status::eReady || out_info.sampler.status() != Status::eReady)
	{
		auto [dSampler, bResult] = res::find<Sampler>("samplers/default");
		if (!bResult)
		{
			LOG_E("[{}] [{}] Failed to locate default sampler", Texture::s_tName, idStr);
			return false;
		}
		out_info.sampler = dSampler;
	}
	if (out_info.sampler.status() != Status::eReady)
	{
		LOG_E("[{}] [{}] Sampler not ready!", Texture::s_tName, idStr);
		return false;
	}
	type = g_texTypes.at((std::size_t)out_createInfo.type);
	colourSpace = g_texModes.at((std::size_t)out_createInfo.mode);
	[[maybe_unused]] bool bAddFileMonitor = false;
	if (!out_createInfo.raws.empty())
	{
		bStbiRaw = false;
		for (auto& raw : out_createInfo.raws)
		{
			raws.push_back(std::move(raw));
		}
	}
	else if (!out_createInfo.bytes.empty())
	{
		for (auto& bytes : out_createInfo.bytes)
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
	else if (!out_createInfo.ids.empty())
	{
		for (auto const& assetID : out_createInfo.ids)
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
	out_info.size = raws.back().size;
	std::vector<Span<u8>> views;
	for (auto const& raw : raws)
	{
		views.push_back(raw.bytes);
	}
	copied = load(active, colourSpace, out_info.size, views, idStr);
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
		for (auto const& id : out_createInfo.ids)
		{
			imgIDs.push_back(id);
			auto onModified = [guid = this->guid, idx](Monitor::File const* pFile) -> bool {
				Texture texture;
				texture.guid = guid;
				if (auto pImpl = res::impl(texture); auto pInfo = res::infoRW(texture))
				{
					auto const idStr = pInfo->id.generic_string();
					auto [raw, bResult] = imgToRaw(pFile->monitor.bytes(), Texture::s_tName, idStr, io::Level::eWarning);
					if (bResult)
					{
						if (pImpl->bStbiRaw)
						{
							stbi_image_free((void*)(pImpl->raws.at(idx).bytes.pData));
						}
						pInfo->size = raw.size;
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
	out_info.mode = out_createInfo.mode;
	out_info.type = out_createInfo.type;
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
	case Status::eError:
	case Status::eReady:
	{
		if (monitor.update())
		{
			auto const idStr = id.generic_string();
			Texture texture;
			texture.guid = guid;
			auto const& info = texture.info();
			std::vector<Span<u8>> views;
			for (auto const& raw : raws)
			{
				views.push_back(raw.bytes);
			}
			copied = load(standby, colourSpace, info.size, views, idStr);
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
}

bool Material::Impl::make(CreateInfo& out_createInfo, Info& out_info)
{
	out_info.albedo = out_createInfo.albedo;
	out_info.shininess = out_createInfo.shininess;
	return true;
}

void Material::Impl::release() {}

bool Mesh::Impl::make(CreateInfo& out_createInfo, Info& out_info)
{
	out_info.material = out_createInfo.material;
	if (out_info.material.material.status() != res::Status::eReady)
	{
		auto [material, bMaterial] = res::findMaterial("materials/default");
		if (!bMaterial)
		{
			return false;
		}
		out_info.material.material = material;
	}
	out_info.material = out_createInfo.material;
	out_info.type = out_createInfo.type;
	updateGeometry(out_info, std::move(out_createInfo.geometry));
	return true;
}

void Mesh::Impl::release()
{
	Mesh mesh;
	mesh.guid = guid;
	if (res::info(mesh).type == Type::eDynamic)
	{
		gfx::vram::unmapMemory(vbo.buffer);
		gfx::vram::unmapMemory(ibo.buffer);
	}
	gfx::deferred::release(vbo.buffer);
	gfx::deferred::release(ibo.buffer);
	vbo = {};
	ibo = {};
}

bool Mesh::Impl::update()
{
	switch (status)
	{
	default:
	{
		return true;
	}
	case Status::eLoading:
	case Status::eReloading:
	{
		auto const vboState = utils::futureState(vbo.copied);
		auto const iboState = utils::futureState(ibo.copied);
		if (vboState == FutureState::eReady && (ibo.count == 0 || iboState == FutureState::eReady))
		{
			return true;
		}
		return false;
	}
	}
}

void Mesh::Impl::updateGeometry(Info& out_info, gfx::Geometry geometry)
{
	if (geometry.vertices.empty())
	{
		status = Status::eReady;
		return;
	}
	auto const idStr = id.generic_string();
	auto const vSize = (vk::DeviceSize)geometry.vertices.size() * sizeof(gfx::Vertex);
	auto const iSize = (vk::DeviceSize)geometry.indices.size() * sizeof(u32);
	auto const bHostVisible = out_info.type == Type::eDynamic;
	if (vSize > vbo.buffer.writeSize)
	{
		if (vbo.buffer.writeSize > 0)
		{
			if (bHostVisible)
			{
				gfx::vram::unmapMemory(vbo.buffer);
			}
			gfx::deferred::release(vbo.buffer);
			vbo = {};
		}
		std::string const name = idStr + "_VBO";
		vbo.buffer = createXBO(name, vSize, vk::BufferUsageFlagBits::eVertexBuffer, bHostVisible);
		if (bHostVisible)
		{
			vbo.pMem = gfx::vram::mapMemory(vbo.buffer, vSize);
		}
	}
	if (iSize > ibo.buffer.writeSize)
	{
		if (ibo.buffer.writeSize > 0)
		{
			if (bHostVisible)
			{
				gfx::vram::unmapMemory(ibo.buffer);
			}
			gfx::deferred::release(ibo.buffer);
			ibo = {};
		}
		std::string const name = idStr + "_IBO";
		ibo.buffer = createXBO(name, iSize, vk::BufferUsageFlagBits::eIndexBuffer, bHostVisible);
		if (bHostVisible)
		{
			ibo.pMem = gfx::vram::mapMemory(ibo.buffer, iSize);
		}
	}
	switch (out_info.type)
	{
	case Type::eStatic:
	{
		vbo.copied = gfx::vram::stage(vbo.buffer, geometry.vertices.data(), vSize);
		if (!geometry.indices.empty())
		{
			ibo.copied = gfx::vram::stage(ibo.buffer, geometry.indices.data(), iSize);
		}
		status = Status::eLoading;
		break;
	}
	case Type::eDynamic:
	{
		std::memcpy(vbo.pMem, geometry.vertices.data(), vSize);
		if (!geometry.indices.empty())
		{
			std::memcpy(ibo.pMem, geometry.indices.data(), iSize);
		}
		status = Status::eReady;
		break;
	}
	default:
		break;
	}
	vbo.count = (u32)geometry.vertices.size();
	ibo.count = (u32)geometry.indices.size();
	out_info.triCount = iSize > 0 ? (u64)ibo.count / 3 : (u64)vbo.count / 3;
}
} // namespace le::res
