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

glm::vec2 textTLOffset(Font::Text::HAlign h, Font::Text::VAlign v)
{
	glm::vec2 textTLoffset = glm::vec2(0.0f);
	switch (h)
	{
	case Font::Text::HAlign::Centre:
	{
		textTLoffset.x = -0.5f;
		break;
	}
	case Font::Text::HAlign::Left:
	default:
		break;

	case Font::Text::HAlign::Right:
	{
		textTLoffset.x = -1.0f;
		break;
	}
	}
	switch (v)
	{
	case Font::Text::VAlign::Middle:
	{
		textTLoffset.y = 0.5f;
		break;
	}
	default:
	case Font::Text::VAlign::Top:
	{
		break;
	}
	case Font::Text::VAlign::Bottom:
	{
		textTLoffset.y = 1.0f;
		break;
	}
	}
	return textTLoffset;
}
} // namespace

std::string const Shader::s_tName = utils::tName<Shader>();
std::string const Sampler::s_tName = utils::tName<Sampler>();
std::string const Texture::s_tName = utils::tName<Texture>();
std::string const Material::s_tName = utils::tName<Material>();
std::string const Mesh::s_tName = utils::tName<Mesh>();
std::string const Font::s_tName = utils::tName<Font>();

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

Font::Info const& Font::info() const
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

Status Font::status() const
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

void Font::Glyph::deserialise(u8 c, GData const& json)
{
	ch = c;
	st = {json.get<s32>("x"), json.get<s32>("y")};
	cell = {json.get<s32>("width"), json.get<s32>("height")};
	uv = cell;
	offset = {json.get<s32>("originX"), json.get<s32>("originY")};
	xAdv = json.contains("advance") ? json.get<s32>("advance") : cell.x;
	orgSizePt = json.get<s32>("size");
	bBlank = json.get<bool>("isBlank");
}

bool Font::CreateInfo::deserialise(GData const& json)
{
	sheetID = json.get("sheetID");
	samplerID = json.contains("sampler") ? json.get("sampler") : "samplers/font";
	materialID = json.contains("material") ? json.get("material") : "materials/default";
	auto glyphsData = json.get<GData>("glyphs");
	for (auto& [key, value] : glyphsData.allFields())
	{
		if (!key.empty())
		{
			Glyph glyph;
			GData data;
			[[maybe_unused]] bool const bSuccess = data.read(std::move(value));
			ASSERT(bSuccess, "Failed to extract glyph!");
			glyph.deserialise((u8)key.at(0), data);
			if (glyph.cell.x > 0 && glyph.cell.y > 0)
			{
				glyphs.push_back(std::move(glyph));
			}
			else
			{
				LOG_W("[{}] Could not deserialise Glyph '{}'!", Font::s_tName, key.at(0));
			}
		}
	}
	return true;
}

gfx::Geometry Font::generate(Text const& text) const
{
	Font font;
	font.guid = guid;
	if (auto pImpl = res::impl(font))
	{
		return pImpl->generate(text);
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
					monitor = {};
					auto pReader = dynamic_cast<io::FileReader const*>(&engine::reader());
					ASSERT(pReader, "FileReader needed!");
					auto onReloaded = [this, idx](Monitor::File const* pFile) -> bool {
						if (!glslToSpirV(pFile->id, codeMap.at(idx)))
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
		for (auto const& resourceID : out_createInfo.ids)
		{
			auto [pixels, bPixels] = engine::reader().getBytes(resourceID);
			if (!bPixels)
			{
				LOG_E("[{}] [{}] Failed to create texture from [{}]!", Texture::s_tName, idStr, resourceID.generic_string());
				return false;
			}
			auto [raw, bResult] = imgToRaw(std::move(pixels), Texture::s_tName, idStr, io::Level::eError);
			if (!bResult)
			{
				LOG_E("[{}] [{}] Failed to create texture from [{}]!", Texture::s_tName, idStr, resourceID.generic_string());
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
			auto onModified = [this, idx](Monitor::File const* pFile) -> bool {
				Texture texture;
				texture.guid = guid;
				if (auto pInfo = res::infoRW(texture))
				{
					auto const idStr = pInfo->id.generic_string();
					auto [raw, bResult] = imgToRaw(pFile->monitor.bytes(), Texture::s_tName, idStr, io::Level::eWarning);
					if (bResult)
					{
						if (bStbiRaw)
						{
							stbi_image_free((void*)(raws.at(idx).bytes.pData));
						}
						pInfo->size = raw.size;
						raws.at(idx) = std::move(raw);
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

bool Font::Impl::make(CreateInfo& out_createInfo, Info& out_info)
{
	res::Texture::CreateInfo sheetInfo;
	stdfs::path texID = id / "sheet";
	if (out_createInfo.samplerID.empty())
	{
		out_createInfo.samplerID = "samplers/font";
	}
	sheetInfo.samplerID = out_createInfo.samplerID;
	sheetInfo.bytes = {std::move(out_createInfo.image)};
	sheet = res::load(texID, std::move(sheetInfo));
	if (sheet.status() == res::Status::eError)
	{
		return false;
	}
	glm::ivec2 maxCell = glm::vec2(0);
	s32 maxXAdv = 0;
	for (auto const& glyph : out_createInfo.glyphs)
	{
		ASSERT(glyph.ch != '\0' && glyphs[(std::size_t)glyph.ch].ch == '\0', "Invalid/duplicate glyph!");
		glyphs.at((std::size_t)glyph.ch) = glyph;
		maxCell.x = std::max(maxCell.x, glyph.cell.x);
		maxCell.y = std::max(maxCell.y, glyph.cell.y);
		maxXAdv = std::max(maxXAdv, glyph.xAdv);
		if (glyph.bBlank)
		{
			blankGlyph = glyph;
		}
	}
	if (blankGlyph.xAdv == 0)
	{
		blankGlyph.cell = maxCell;
		blankGlyph.xAdv = maxXAdv;
	}
	material = out_createInfo.material;
	if (material.material.status() != res::Status::eReady)
	{
		auto [mat, bMat] = res::findMaterial("materials/default");
		if (bMat)
		{
			material.material = mat;
		}
	}
	ASSERT(material.material.status() == res::Status::eReady, "Material is not ready!");
	material.diffuse = sheet;
	material.flags.set({res::Material::Flag::eTextured, res::Material::Flag::eUI, res::Material::Flag::eDropColour});
	material.flags.reset({res::Material::Flag::eOpaque, res::Material::Flag::eLit});
	out_info.material = material;
	out_info.sheet = sheet;
	return true;
}

void Font::Impl::release()
{
	res::unload(sheet);
	sheet = {};
}

bool Font::Impl::update()
{
	switch (status)
	{
	case Status::eLoading:
	case Status::eReloading:
	{
		auto const status = sheet.status();
		return status == Status::eReady || status == Status::eError;
	}
	default:
	{
		return true;
	}
	}
}

gfx::Geometry Font::Impl::generate(Text const& text) const
{
	Font font;
	font.guid = guid;
	if (text.text.empty() || font.status() != Status::eReady)
	{
		return {};
	}
	glm::ivec2 maxCell = glm::vec2(0);
	for (auto c : text.text)
	{
		maxCell.x = std::max(maxCell.x, glyphs.at((std::size_t)c).cell.x);
		maxCell.y = std::max(maxCell.y, glyphs.at((std::size_t)c).cell.y);
	}
	u32 lineCount = 1;
	for (std::size_t idx = 0; idx < text.text.size(); ++idx)
	{
		if (text.text[idx] == '\n')
		{
			++lineCount;
		}
	}
	f32 const lineHeight = ((f32)maxCell.y) * text.scale;
	f32 const linePad = lineHeight * text.nYPad;
	f32 const textHeight = (f32)lineCount * lineHeight;
	glm::vec2 const realTopLeft = text.pos;
	glm::vec2 textTL = realTopLeft;
	std::size_t nextLineIdx = 0;
	s32 yIdx = 0;
	f32 xPos = 0.0f;
	f32 lineWidth = 0.0f;
	auto const textTLoffset = textTLOffset(text.halign, text.valign);
	auto updateTextTL = [&]() {
		lineWidth = 0.0f;
		for (; nextLineIdx < text.text.size(); ++nextLineIdx)
		{
			auto const ch = text.text.at(nextLineIdx);
			if (ch == '\n')
			{
				break;
			}
			else
			{
				lineWidth += (f32)glyphs.at((std::size_t)ch).xAdv;
			}
		}
		lineWidth *= text.scale;
		++nextLineIdx;
		xPos = 0.0f;
		textTL = realTopLeft + textTLoffset * glm::vec2(lineWidth, textHeight);
		textTL.y -= (lineHeight + ((f32)yIdx * (lineHeight + linePad)));
	};
	updateTextTL();

	gfx::Geometry ret;
	u32 quadCount = (u32)text.text.length();
	ret.reserve(4 * quadCount, 6 * quadCount);
	auto const normal = glm::vec3(0.0f);
	auto const colour = glm::vec3(1.0f);
	auto const texSize = sheet.info().size;
	for (auto const c : text.text)
	{
		if (c == '\n')
		{
			++yIdx;
			updateTextTL();
			continue;
		}
		auto const& search = glyphs.at((std::size_t)c);
		auto const& glyph = search.ch == '\0' ? blankGlyph : search;
		auto const offset = glm::vec3(xPos - (f32)glyph.offset.x * text.scale, (f32)glyph.offset.y * text.scale, 0.0f);
		auto const tl = glm::vec3(textTL.x, textTL.y, text.pos.z) + offset;
		auto const s = (f32)glyph.st.x / (f32)texSize.x;
		auto const t = (f32)glyph.st.y / (f32)texSize.y;
		auto const u = s + (f32)glyph.uv.x / (f32)texSize.x;
		auto const v = t + (f32)glyph.uv.y / (f32)texSize.y;
		glm::vec2 const cell = {(f32)glyph.cell.x * text.scale, (f32)glyph.cell.y * text.scale};
		auto const v0 = ret.addVertex({tl, colour, normal, glm::vec2(s, t)});
		auto const v1 = ret.addVertex({tl + glm::vec3(cell.x, 0.0f, 0.0f), colour, normal, glm::vec2(u, t)});
		auto const v2 = ret.addVertex({tl + glm::vec3(cell.x, -cell.y, 0.0f), colour, normal, glm::vec2(u, v)});
		auto const v3 = ret.addVertex({tl + glm::vec3(0.0f, -cell.y, 0.0f), colour, normal, glm::vec2(s, v)});
		ret.addIndices({v0, v1, v2, v2, v3, v0});
		xPos += ((f32)glyph.xAdv * text.scale);
	}
	return ret;
}
} // namespace le::res
