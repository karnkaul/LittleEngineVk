#include <stb/stb_image.h>
#include <core/log.hpp>
#include <engine/resources/resources.hpp>
#include <engine/resources/shader_compiler.hpp>
#include <gfx/common.hpp>
#include <gfx/deferred.hpp>
#include <gfx/device.hpp>
#include <gfx/vram.hpp>
#include <levk_impl.hpp>
#include <resources/resources_impl.hpp>

namespace le::res {
namespace {
std::array const g_filters = {vk::Filter::eLinear, vk::Filter::eNearest};
std::array const g_mipModes = {vk::SamplerMipmapMode::eLinear, vk::SamplerMipmapMode::eNearest};
std::array const g_samplerModes = {vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToBorder};
std::array const g_texModes = {vk::Format::eR8G8B8A8Srgb, vk::Format::eR8G8B8A8Snorm};
std::array const g_texTypes = {vk::ImageViewType::e2D, vk::ImageViewType::eCube};

std::future<void> load(gfx::Image& out_image, vk::Format texMode, glm::ivec2 const& size, Span<Span<u8>> bytes, [[maybe_unused]] std::string_view name) {
	if (out_image.image == vk::Image() || out_image.extent.width != (u32)size.x || out_image.extent.height != (u32)size.y) {
		gfx::ImageInfo imageInfo;
		imageInfo.queueFlags = gfx::QFlag::eTransfer | gfx::QFlag::eGraphics;
		imageInfo.createInfo.format = texMode;
		imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.createInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		if (bytes.extent > 1) {
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

Result<Texture::Raw> imgToRaw(bytearray imgBytes, std::string_view tName, std::string_view id, dl::level errLevel) {
	Texture::Raw ret;
	s32 ch;
	auto pIn = reinterpret_cast<stbi_uc const*>(imgBytes.data());
	auto pOut = stbi_load_from_memory(pIn, (s32)imgBytes.size(), &ret.size.x, &ret.size.y, &ch, 4);
	if (!pOut) {
		dl::log(errLevel, "[{}] [{}] Failed to load image data!", tName, id);
		return {};
	}
	std::size_t const size = (std::size_t)(ret.size.x * ret.size.y * 4);
	ret.bytes = Span(pOut, size);
	return ret;
}

gfx::Buffer createXBO(std::string_view name, vk::DeviceSize size, vk::BufferUsageFlagBits usage, bool bHostVisible) {
	gfx::BufferInfo bufferInfo;
	bufferInfo.size = size;
	if (bHostVisible) {
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		bufferInfo.queueFlags = gfx::QFlag::eGraphics;
		bufferInfo.share = vk::SharingMode::eExclusive;
	} else {
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		bufferInfo.queueFlags = gfx::QFlag::eGraphics | gfx::QFlag::eTransfer;
	}
	bufferInfo.usage = usage | vk::BufferUsageFlagBits::eTransferDst;
	bufferInfo.name = name;
	return gfx::vram::createBuffer(bufferInfo);
}

glm::vec2 textTLOffset(Font::Text::HAlign h, Font::Text::VAlign v) {
	glm::vec2 textTLoffset = glm::vec2(0.0f);
	switch (h) {
	case Font::Text::HAlign::Centre: {
		textTLoffset.x = -0.5f;
		break;
	}
	case Font::Text::HAlign::Left:
	default:
		break;

	case Font::Text::HAlign::Right: {
		textTLoffset.x = -1.0f;
		break;
	}
	}
	switch (v) {
	case Font::Text::VAlign::Middle: {
		textTLoffset.y = 0.5f;
		break;
	}
	default:
	case Font::Text::VAlign::Top: {
		break;
	}
	case Font::Text::VAlign::Bottom: {
		textTLoffset.y = 1.0f;
		break;
	}
	}
	return textTLoffset;
}
} // namespace

Shader::Info const& Shader::info() const {
	return res::info(*this);
}

Sampler::Info const& Sampler::info() const {
	return res::info(*this);
}

Texture::Info const& Texture::info() const {
	return res::info(*this);
}

Material::Info const& Material::info() const {
	return res::info(*this);
}

Mesh::Info const& Mesh::info() const {
	return res::info(*this);
}

Font::Info const& Font::info() const {
	return res::info(*this);
}

Status Shader::status() const {
	return res::status(*this);
}

Status Sampler::status() const {
	return res::status(*this);
}

Status Texture::status() const {
	return res::status(*this);
}

Status Material::status() const {
	return res::status(*this);
}

Status Mesh::status() const {
	return res::status(*this);
}

Status Font::status() const {
	return res::status(*this);
}

void Mesh::updateGeometry(gfx::Geometry geometry) {
	if (auto pImpl = res::impl(*this); auto pInfo = res::infoRW(*this)) {
		pImpl->updateGeometry(*pInfo, std::move(geometry));
	}
}

template <>
kt::result_void<Texture::CreateInfo> LoadBase<Texture>::createInfo() const {
	auto pThis = static_cast<Texture::LoadInfo const*>(this);
	if (pThis->imageFilename.empty() && pThis->cubemapFilenames.empty()) {
		logE("[{}] Empty resource ID(s)!", Texture::s_tName);
		return {};
	}
	Texture::CreateInfo ret;
	ret.mode = engine::colourSpace();
	ret.samplerID = pThis->samplerID;
	if (!pThis->cubemapFilenames.empty()) {
		if (pThis->cubemapFilenames.size() != 6) {
			logE("[{}] Invalid cubemap filename count [{}]!", Texture::s_tName, pThis->cubemapFilenames.size());
			return {};
		}
		ret.type = Texture::Type::eCube;
		for (auto const& name : pThis->cubemapFilenames) {
			io::Path const id = pThis->directory / name;
			if (engine::reader().checkPresence(id)) {
				ret.ids.push_back(id);
			} else {
				logE("[{}] Resource ID [{}] not found in [{}]!", Texture::s_tName, id.generic_string(), engine::reader().medium());
				return {};
			}
		}
	} else {
		ret.type = Texture::Type::e2D;
		auto const id = pThis->directory / pThis->imageFilename;
		if (!engine::reader().checkPresence(id)) {
			logE("[{}] Resource ID [{}] not found in [{}]!", Texture::s_tName, id.generic_string(), engine::reader().medium());
			return {};
		}
		ret.ids = {id};
	}
	return ret;
}

Material::Inst& Mesh::material() {
	static Material::Inst s_default{};
	if (auto pInfo = res::infoRW(*this)) {
		return pInfo->material;
	}
	return s_default;
}

std::string Shader::Impl::extension(io::Path const& id) {
	auto const str = id.generic_string();
	if (auto idx = str.find_last_of('.'); idx != std::string::npos) {
		return str.substr(idx);
	}
	return {};
}

void Font::Glyph::deserialise(u8 c, dj::node_t const& json) {
	ch = c;
	st = {json.get("x").as<s32>(), json.get("y").as<s32>()};
	uv = cell = {json.get("width").as<s32>(), json.get("height").as<s32>()};
	offset = {json.get("originX").as<s32>(), json.get("originY").as<s32>()};
	auto const pAdvance = json.find("advance");
	xAdv = pAdvance ? pAdvance->as<s32>() : cell.x;
	if (auto pBlank = json.find("isBlank")) {
		bBlank = pBlank->as<bool>();
	}
}

bool Font::CreateInfo::deserialise(std::string const& jsonStr) {
	if (auto json = dj::node_t::make(jsonStr)) {
		if (auto pAtlas = json->find("sheetID")) {
			sheetID = pAtlas->as<std::string>();
		}
		if (auto pSampler = json->find("samplerID")) {
			samplerID = pSampler->as<std::string>();
		}
		if (auto pMaterial = json->find("materialID")) {
			materialID = pMaterial->as<std::string>();
		}
		if (auto pGlyphsData = json->find("glyphs")) {
			for (auto& [key, value] : pGlyphsData->as<dj::map_nodes_t>()) {
				if (!key.empty()) {
					Glyph glyph;
					glyph.deserialise((u8)key[0], *value);
					if (glyph.cell.x > 0 && glyph.cell.y > 0) {
						glyphs.push_back(glyph);
					} else {
						logW("Could not deserialise Glyph '{}'!", key[0]);
					}
				}
			}
		}
		return true;
	}
	return false;
}

gfx::Geometry Font::generate(Text const& text, std::optional<Layout> layout) const {
	if (auto pImpl = res::impl(*this)) {
		return pImpl->generate(text, layout);
	}
	return {};
}

glm::ivec2 Font::glyphBounds(std::string_view text) const {
	if (auto pImpl = res::impl(*this)) {
		return pImpl->glyphBounds(text);
	}
	return {};
}

Font::Layout Font::layout(std::string_view text, Size size, f32 nPadY) const {
	if (auto pImpl = res::impl(*this)) {
		return pImpl->layout(text, size, nPadY);
	}
	return {};
}

bool Shader::Impl::make(CreateInfo& out_createInfo, Info&) {
	bool const bCodeMapPopulated = std::any_of(out_createInfo.codeMap.begin(), out_createInfo.codeMap.end(), [&](auto const& entry) { return !entry.empty(); });
	[[maybe_unused]] bool const bCodeIDsPopulated =
		std::any_of(out_createInfo.codeIDMap.begin(), out_createInfo.codeIDMap.end(), [&](auto const& entry) { return !entry.empty(); });
	if (bCodeMapPopulated) {
		codeMap = std::move(out_createInfo.codeMap);
	} else {
		ENSURE(bCodeIDsPopulated, "Invalid Shader ShaderData!");
		[[maybe_unused]] auto pReader = dynamic_cast<io::FileReader const*>(&engine::reader());
		for (std::size_t idx = 0; idx < out_createInfo.codeIDMap.size(); ++idx) {
			auto& codeID = out_createInfo.codeIDMap[idx];
			auto const ext = extension(codeID);
			bool bSpv = true;
			if (ext == s_vertExt || ext == s_fragExt) {
				if (!levk_shaderCompiler || !pReader) {
					codeID += s_spvExt;
				} else {
#if defined(LEVK_SHADER_COMPILER)
					if (loadGlsl(codeID, (Type)idx)) {
#if defined(LEVK_RESOURCES_HOT_RELOAD)
						if (pReader) {
							monitor = {};
							auto onReloaded = [this, idx](Monitor::File const& file) -> bool {
								if (!glslToSpirV(file.id, codeMap[idx])) {
									logE("[{}] Failed to reload Shader!", s_tName);
									return false;
								}
								return true;
							};
							monitor.m_files.push_back({codeID, pReader->fullPath(codeID), io::FileMonitor::Mode::eTextContents, onReloaded});
						}
#endif
					} else {
						logE("[{}] Failed to compile GLSL code to SPIR-V!", s_tName);
						return false;
					}
					bSpv = false;
#endif
				}
			}
			if (bSpv) {
				auto shaderData = engine::reader().bytes(codeID);
				ENSURE(shaderData, "Shader code missing!");
				if (!shaderData) {
					logE("[{}] [{}] Shader code missing: [{}]!", s_tName, id.generic_string(), codeID.generic_string());
					return false;
				}
				codeMap[idx] = std::move(*shaderData);
			}
		}
	}
	return loadAllSpirV();
}

void Shader::Impl::release() {
	for (auto const& shader : shaders) {
		gfx::g_device.destroy(shader);
	}
}

#if defined(LEVK_RESOURCES_HOT_RELOAD)
bool Shader::Impl::checkReload() {
	switch (status) {
	case Status::eError:
	case Status::eReady: {
		if (monitor.update()) {
			auto const idStr = id.generic_string();
			logD("[{}] [{}] Reloading...", s_tName, idStr);
			if (loadAllSpirV()) {
				logI("[{}] [{}] Reloaded", s_tName, idStr);
				return true;
			} else {
				logE("[{}] [{}] Failed to reload!", s_tName, idStr);
				return false;
			}
		}
		return false;
	}
	default: {
		return false;
	}
	}
}
#endif

#if defined(LEVK_SHADER_COMPILER)
bool Shader::Impl::loadGlsl(io::Path const& id, Type type) {
	if (ShaderCompiler::instance().status() != ShaderCompiler::Status::eOnline) {
		logE("[{}] ShaderCompiler is Offline!", s_tName);
		return false;
	}
	return engine::reader().isPresent(id) && glslToSpirV(id, codeMap[(std::size_t)type]);
}

bool Shader::Impl::glslToSpirV(io::Path const& id, bytearray& out_bytes) {
	if (ShaderCompiler::instance().status() != ShaderCompiler::Status::eOnline) {
		logE("[{}] ShaderCompiler is Offline!", s_tName);
		return false;
	}
	auto pReader = dynamic_cast<io::FileReader const*>(&engine::reader());
	ENSURE(pReader, "Cannot compile shaders without io::FileReader!");
	if (pReader->isPresent(id)) {
		auto const src = pReader->fullPath(id);
		auto dstID = id;
		dstID += s_spvExt;
		if (!ShaderCompiler::instance().compile(src, true)) {
			return false;
		}
		auto spvCode = pReader->bytes(dstID);
		if (!spvCode) {
			return false;
		}
		out_bytes = std::move(*spvCode);
		return true;
	}
	return false;
}
#endif

bool Shader::Impl::loadAllSpirV() {
	bool bFound = false;
	for (std::size_t idx = 0; idx < codeMap.size(); ++idx) {
		auto const& code = codeMap[idx];
		if (!code.empty()) {
			bFound = true;
			gfx::g_device.destroy(shaders[idx]);
			vk::ShaderModuleCreateInfo createInfo;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<u32 const*>(code.data());
			shaders[idx] = gfx::g_device.device.createShaderModule(createInfo);
		}
	}
	return bFound;
}

vk::ShaderModule Shader::Impl::module(Shader::Type type) const {
	ENSURE(shaders[(std::size_t)type] != vk::ShaderModule(), "Module not present in Shader!");
	return shaders[(std::size_t)type];
}

std::map<Shader::Type, vk::ShaderModule> Shader::Impl::modules() const {
	std::map<Shader::Type, vk::ShaderModule> ret;
	for (std::size_t idx = 0; idx < (std::size_t)Shader::Type::eCOUNT_; ++idx) {
		auto const& module = shaders[idx];
		if (module != vk::ShaderModule()) {
			ret[(Shader::Type)idx] = module;
		}
	}
	return ret;
}

bool Sampler::Impl::make(CreateInfo& out_createInfo, Info& out_info) {
	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = g_filters[(std::size_t)out_createInfo.min];
	samplerInfo.minFilter = g_filters[(std::size_t)out_createInfo.mag];
	samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = g_samplerModes[(std::size_t)out_createInfo.mode];
	samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
	samplerInfo.unnormalizedCoordinates = false;
	samplerInfo.compareEnable = false;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	samplerInfo.mipmapMode = g_mipModes[(std::size_t)out_createInfo.mip];
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

void Sampler::Impl::release() {
	gfx::deferred::release([sampler = this->sampler]() { gfx::g_device.destroy(sampler); });
	sampler = vk::Sampler();
}

bool Texture::Impl::make(CreateInfo& out_createInfo, Info& out_info) {
	auto const idStr = id.generic_string();
	auto sampler = res::find<Sampler>(out_createInfo.samplerID);
	if (sampler && sampler->status() == Status::eReady) {
		out_info.sampler = *sampler;
	} else {
		auto dSampler = res::find<Sampler>("samplers/default");
		if (!dSampler) {
			logE("[{}] [{}] Failed to locate default sampler", Texture::s_tName, idStr);
			return false;
		}
		out_info.sampler = *dSampler;
	}
	if (out_info.sampler.status() != Status::eReady) {
		logE("[{}] [{}] Sampler not ready!", Texture::s_tName, idStr);
		return false;
	}
	type = g_texTypes[(std::size_t)out_createInfo.type];
	colourSpace = g_texModes[(std::size_t)out_createInfo.mode];
	[[maybe_unused]] bool bAddFileMonitor = false;
	if (!out_createInfo.raws.empty()) {
		bStbiRaw = false;
		for (auto& raw : out_createInfo.raws) {
			raws.push_back(std::move(raw));
		}
	} else if (!out_createInfo.bytes.empty()) {
		for (auto& bytes : out_createInfo.bytes) {
			auto raw = imgToRaw(std::move(bytes), Texture::s_tName, idStr, dl::level::error);
			if (!raw) {
				logE("[{}] [{}] Failed to create texture!", Texture::s_tName, idStr);
				return false;
			}
			raws.push_back(std::move(*raw));
		}
		bStbiRaw = true;
	} else if (!out_createInfo.ids.empty()) {
		for (auto const& resourceID : out_createInfo.ids) {
			auto pixels = engine::reader().bytes(resourceID);
			if (!pixels) {
				logE("[{}] [{}] Failed to create texture from [{}]!", Texture::s_tName, idStr, resourceID.generic_string());
				return false;
			}
			auto raw = imgToRaw(std::move(*pixels), Texture::s_tName, idStr, dl::level::error);
			if (!raw) {
				logE("[{}] [{}] Failed to create texture from [{}]!", Texture::s_tName, idStr, resourceID.generic_string());
				return false;
			}
			raws.push_back(std::move(*raw));
		}
		bStbiRaw = true;
		bAddFileMonitor = true;
	} else {
		ENSURE(false, "Invalid Info!");
		logE("[{}] [{}] Invalid Texture Info!", Texture::s_tName, idStr);
		return false;
	}
	out_info.size = raws.back().size;
	spanRaws.clear();
	for (auto const& raw : raws) {
		spanRaws.push_back(raw.bytes);
	}
	copied = load(active, colourSpace, out_info.size, spanRaws, idStr);
	gfx::ImageViewInfo viewInfo;
	viewInfo.image = active.image;
	viewInfo.format = colourSpace;
	viewInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
	viewInfo.type = type;
	imageView = gfx::g_device.createImageView(viewInfo);
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	auto pReader = dynamic_cast<io::FileReader const*>(&engine::reader());
	monitor = {};
	monitor.m_reloadDelay = 50ms;
	if (bAddFileMonitor && pReader) {
		std::size_t idx = 0;
		for (auto const& id : out_createInfo.ids) {
			imgIDs.push_back(id);
			auto onModified = [this, idx](Monitor::File const& file) -> bool {
				Texture texture;
				texture.guid = guid;
				if (auto pInfo = res::infoRW(texture)) {
					auto const idStr = pInfo->id.generic_string();
					auto raw = imgToRaw(file.monitor.bytes(), Texture::s_tName, idStr, dl::level::warning);
					if (raw) {
						if (bStbiRaw) {
							stbi_image_free((void*)(raws[idx].bytes.pData));
						}
						pInfo->size = raw->size;
						raws[idx] = std::move(*raw);
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

bool Texture::Impl::update() {
	switch (status) {
	case Status::eReady:
	case Status::eError: {
		return true;
	}
	case Status::eLoading:
	case Status::eReloading: {
		if (utils::futureState(copied) == FutureState::eReady) {
			auto const idStr = id.generic_string();
			auto const szLoadStr = status == Status::eReloading ? "reloaded" : "loaded";
			logI("[{}] [{}] {}", Texture::s_tName, idStr, szLoadStr);
#if defined(LEVK_RESOURCES_HOT_RELOAD)
			if (status == Status::eReloading) {
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
	default: {
		return true;
	}
	}
}

#if defined(LEVK_RESOURCES_HOT_RELOAD)
bool Texture::Impl::checkReload() {
	switch (status) {
	case Status::eError:
	case Status::eReady: {
		if (monitor.update()) {
			auto const idStr = id.generic_string();
			Texture texture;
			texture.guid = guid;
			auto const& info = texture.info();
			spanRaws.clear();
			for (auto const& raw : raws) {
				spanRaws.push_back(raw.bytes);
			}
			copied = load(standby, colourSpace, info.size, spanRaws, idStr);
			return true;
		}
		return false;
	}
	default: {
		return false;
	}
	}
}
#endif

void Texture::Impl::release() {
	for (auto& raw : raws) {
		if (bStbiRaw && raw.bytes.pData) {
			stbi_image_free((void*)(raw.bytes.pData));
		}
	}
	gfx::deferred::release(active, imageView);
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	if (standby.image != active.image) {
		gfx::deferred::release(standby);
	}
	standby = {};
#endif
	active = {};
	imageView = vk::ImageView();
}

bool Material::Impl::make(CreateInfo& out_createInfo, Info& out_info) {
	out_info.albedo = out_createInfo.albedo;
	out_info.shininess = out_createInfo.shininess;
	return true;
}

void Material::Impl::release() {
}

bool Mesh::Impl::make(CreateInfo& out_createInfo, Info& out_info) {
	out_info.material = out_createInfo.material;
	if (out_info.material.material.status() != res::Status::eReady) {
		auto material = res::find<Material>("materials/default");
		if (!material) {
			return false;
		}
		out_info.material.material = *material;
	}
	out_info.type = out_createInfo.type;
	updateGeometry(out_info, std::move(out_createInfo.geometry));
	return true;
}

void Mesh::Impl::release() {
	Mesh mesh;
	mesh.guid = guid;
	if (res::info(mesh).type == Type::eDynamic) {
		gfx::vram::unmapMemory(vbo.buffer);
		gfx::vram::unmapMemory(ibo.buffer);
	}
	gfx::deferred::release(vbo.buffer);
	gfx::deferred::release(ibo.buffer);
	vbo = {};
	ibo = {};
}

bool Mesh::Impl::update() {
	switch (status) {
	default: {
		return true;
	}
	case Status::eLoading:
	case Status::eReloading: {
		auto const vboState = utils::futureState(vbo.copied);
		auto const iboState = utils::futureState(ibo.copied);
		if (vboState == FutureState::eReady && (ibo.count == 0 || iboState == FutureState::eReady)) {
			return true;
		}
		return false;
	}
	}
}

void Mesh::Impl::updateGeometry(Info& out_info, gfx::Geometry geometry) {
	if (geometry.vertices.empty()) {
		status = Status::eReady;
		return;
	}
	geo = std::move(geometry);
	auto const idStr = id.generic_string();
	auto const vSize = (vk::DeviceSize)geo.vertices.size() * sizeof(gfx::Vertex);
	auto const iSize = (vk::DeviceSize)geo.indices.size() * sizeof(u32);
	auto const bHostVisible = out_info.type == Type::eDynamic;
	if (vSize > vbo.buffer.writeSize) {
		if (vbo.buffer.writeSize > 0) {
			gfx::deferred::release(vbo.buffer);
			vbo = {};
		}
		std::string const name = idStr + "_VBO";
		vbo.buffer = createXBO(name, vSize, vk::BufferUsageFlagBits::eVertexBuffer, bHostVisible);
		if (bHostVisible) {
			[[maybe_unused]] bool const bResult = gfx::vram::mapMemory(vbo.buffer);
			ENSURE(bResult, "Memory map failed");
		}
	}
	if (iSize > ibo.buffer.writeSize) {
		if (ibo.buffer.writeSize > 0) {
			gfx::deferred::release(ibo.buffer);
			ibo = {};
		}
		std::string const name = idStr + "_IBO";
		ibo.buffer = createXBO(name, iSize, vk::BufferUsageFlagBits::eIndexBuffer, bHostVisible);
		if (bHostVisible) {
			[[maybe_unused]] bool const bResult = gfx::vram::mapMemory(ibo.buffer);
			ENSURE(bResult, "Memory map failed");
		}
	}
	switch (out_info.type) {
	case Type::eStatic: {
		vbo.copied = gfx::vram::stage(vbo.buffer, geo.vertices.data(), vSize);
		if (!geo.indices.empty()) {
			ibo.copied = gfx::vram::stage(ibo.buffer, geo.indices.data(), iSize);
		}
		status = Status::eLoading;
		break;
	}
	case Type::eDynamic: {
		std::memcpy(vbo.buffer.pMap, geo.vertices.data(), vSize);
		if (!geo.indices.empty()) {
			std::memcpy(ibo.buffer.pMap, geo.indices.data(), iSize);
		}
		status = Status::eReady;
		break;
	}
	default:
		break;
	}
	vbo.count = (u32)geo.vertices.size();
	ibo.count = (u32)geo.indices.size();
	out_info.triCount = iSize > 0 ? (u64)ibo.count / 3 : (u64)vbo.count / 3;
}

bool Font::Impl::make(CreateInfo& out_createInfo, Info& out_info) {
	[[maybe_unused]] bool bAddMonitor = false;
	if (out_createInfo.sheetID.empty() || out_createInfo.glyphs.empty()) {
		out_info.jsonID = out_createInfo.jsonID;
		auto json = engine::reader().string(out_createInfo.jsonID);
		if (!json || !out_createInfo.deserialise(*json)) {
			logE("[{}] [{}] Invalid Font data", s_tName, id.generic_string());
			return false;
		}
		auto img = engine::reader().bytes("fonts" / out_createInfo.sheetID);
		if (!img) {
			logE("[{}] [{}] Failed to load font atlas!", s_tName, id.generic_string());
			return false;
		}
		out_createInfo.image = std::move(*img);
		if (auto material = res::find<Material>(out_createInfo.materialID)) {
			out_createInfo.material.material = *material;
		}
		bAddMonitor = true;
	}
	res::Texture::CreateInfo sheetInfo;
	io::Path texID = id / "sheet";
	if (out_createInfo.samplerID == Hash()) {
		out_createInfo.samplerID = "samplers/font";
	}
	sheetInfo.samplerID = out_createInfo.samplerID;
	sheetInfo.bytes = {std::move(out_createInfo.image)};
	sheet = res::load(texID, std::move(sheetInfo));
	if (sheet.status() == res::Status::eError) {
		return false;
	}
	loadGlyphs(out_createInfo.glyphs, false);
	material = out_createInfo.material;
	if (material.material.status() != res::Status::eReady) {
		if (auto mat = res::find<Material>("materials/default")) {
			material.material = *mat;
		}
	}
	ENSURE(material.material.status() == res::Status::eReady, "Material is not ready!");
	material.diffuse = sheet;
	material.flags.set(res::Material::Flag::eTextured | res::Material::Flag::eUI | res::Material::Flag::eDropColour);
	material.flags.reset(res::Material::Flag::eOpaque | res::Material::Flag::eLit);
	out_info.material = material;
	out_info.sheet = sheet;
	out_info.maxBounds = {};
	for (auto const& glyph : glyphs) {
		out_info.maxBounds.x = std::max(out_info.maxBounds.x, glyph.cell.x);
		out_info.maxBounds.y = std::max(out_info.maxBounds.y, glyph.cell.y);
	}
#if defined(LEVK_RESOURCES_HOT_RELOAD)
	auto pReader = dynamic_cast<io::FileReader const*>(&engine::reader());
	monitor = {};
	if (bAddMonitor && pReader) {
		monitor.m_files.push_back({id, pReader->fullPath(out_info.jsonID), io::FileMonitor::Mode::eTextContents, [](auto) { return true; }});
	}
#endif
	return true;
}

void Font::Impl::release() {
	res::unload(sheet);
	sheet = {};
}

bool Font::Impl::update() {
	switch (status) {
	case Status::eLoading:
	case Status::eReloading: {
		auto const status = sheet.status();
		return status == Status::eReady || status == Status::eError;
	}
	default: {
		return true;
	}
	}
}

#if defined(LEVK_RESOURCES_HOT_RELOAD)
bool Font::Impl::checkReload() {
	switch (status) {
	case Status::eReady:
	case Status::eError: {
		if (monitor.update()) {
			Font font;
			font.guid = guid;
			auto pInfo = res::infoRW(font);
			if (pInfo) {
				auto json = engine::reader().string(pInfo->jsonID);
				Font::CreateInfo createInfo;
				if (json && createInfo.deserialise(*json)) {
					loadGlyphs(createInfo.glyphs, true);
					return true;
				}
			}
		}
		return false;
	}
	default: {
		return false;
	}
	}
}
#endif

void Font::Impl::loadGlyphs(std::vector<Glyph> const& glyphData, [[maybe_unused]] bool bOverwrite) {
	glm::ivec2 maxCell = glm::vec2(0);
	s32 maxXAdv = 0;
	for (auto const& glyph : glyphData) {
		ENSURE(glyph.ch != '\0' && (bOverwrite || glyphs[(std::size_t)glyph.ch].ch == '\0'), "Invalid/duplicate glyph!");
		glyphs[(std::size_t)glyph.ch] = glyph;
		maxCell.x = std::max(maxCell.x, glyph.cell.x);
		maxCell.y = std::max(maxCell.y, glyph.cell.y);
		maxXAdv = std::max(maxXAdv, glyph.xAdv);
		if (glyph.bBlank) {
			blankGlyph = glyph;
		}
	}
	if (blankGlyph.xAdv == 0) {
		blankGlyph.cell = maxCell;
		blankGlyph.xAdv = maxXAdv;
	}
}

gfx::Geometry Font::Impl::generate(Text const& text, std::optional<Font::Layout> layout) const {
	if (text.text.empty()) {
		return {};
	}
	if (!layout) {
		layout = this->layout(text.text, text.size, text.nYPad);
	}
	glm::vec2 const realTopLeft = text.pos;
	glm::vec2 textTL = realTopLeft;
	std::size_t nextLineIdx = 0;
	s32 yIdx = 0;
	f32 xPos = 0.0f;
	f32 lineWidth = 0.0f;
	auto const textTLoffset = textTLOffset(text.halign, text.valign);
	auto updateTextTL = [&]() {
		lineWidth = 0.0f;
		f32 maxOffsetY = 0.0f;
		for (; nextLineIdx < text.text.size(); ++nextLineIdx) {
			auto const ch = text.text[nextLineIdx];
			if (ch == '\n') {
				break;
			} else {
				Glyph const& glyph = glyphs[(std::size_t)ch];
				lineWidth += (f32)glyph.xAdv;
				maxOffsetY = std::max(maxOffsetY, (f32)glyph.offset.y);
			}
		}
		maxOffsetY *= layout->scale;
		f32 const offsetY = layout->lineHeight - maxOffsetY;
		lineWidth *= layout->scale;
		++nextLineIdx;
		xPos = 0.0f;
		textTL = realTopLeft + textTLoffset * glm::vec2(lineWidth, layout->textHeight + offsetY);
		textTL.y -= (layout->lineHeight + ((f32)yIdx * (layout->lineHeight + layout->linePad)));
	};
	updateTextTL();

	gfx::Geometry ret;
	u32 quadCount = (u32)text.text.length();
	ret.reserve(4 * quadCount, 6 * quadCount);
	auto const normal = glm::vec3(0.0f);
	auto const colour = glm::vec3(1.0f);
	auto const texSize = sheet.info().size;
	for (auto const c : text.text) {
		if (c == '\n') {
			++yIdx;
			updateTextTL();
			continue;
		}
		auto const& search = glyphs[(std::size_t)c];
		auto const& glyph = search.ch == '\0' ? blankGlyph : search;
		auto const offset = glm::vec3(xPos - (f32)glyph.offset.x * layout->scale, (f32)glyph.offset.y * layout->scale, 0.0f);
		auto const tl = glm::vec3(textTL.x, textTL.y, text.pos.z) + offset;
		auto const s = (f32)glyph.st.x / (f32)texSize.x;
		auto const t = (f32)glyph.st.y / (f32)texSize.y;
		auto const u = s + (f32)glyph.uv.x / (f32)texSize.x;
		auto const v = t + (f32)glyph.uv.y / (f32)texSize.y;
		glm::vec2 const cell = {(f32)glyph.cell.x * layout->scale, (f32)glyph.cell.y * layout->scale};
		auto const v0 = ret.addVertex({tl, colour, normal, glm::vec2(s, t)});
		auto const v1 = ret.addVertex({tl + glm::vec3(cell.x, 0.0f, 0.0f), colour, normal, glm::vec2(u, t)});
		auto const v2 = ret.addVertex({tl + glm::vec3(cell.x, -cell.y, 0.0f), colour, normal, glm::vec2(u, v)});
		auto const v3 = ret.addVertex({tl + glm::vec3(0.0f, -cell.y, 0.0f), colour, normal, glm::vec2(s, v)});
		ret.addIndices({v0, v1, v2, v2, v3, v0});
		xPos += ((f32)glyph.xAdv * layout->scale);
	}
	return ret;
}

glm::ivec2 Font::Impl::glyphBounds(std::string_view text) const {
	glm::ivec2 ret = {};
	if (text.empty()) {
		Font font;
		font.guid = guid;
		ret = res::info(font).maxBounds;
	} else {
		for (char c : text) {
			ret.x = std::max(ret.x, glyphs[(std::size_t)c].cell.x);
			ret.y = std::max(ret.y, glyphs[(std::size_t)c].cell.y);
		}
	}
	return ret;
}

Font::Layout Font::Impl::layout(std::string_view text, Font::Size size, f32 nPadY) const {
	Font::Layout ret;
	ret.maxBounds = glyphBounds(text);
	ret.lineCount = 1;
	for (std::size_t idx = 0; idx < text.size(); ++idx) {
		if (text[idx] == '\n') {
			++ret.lineCount;
		}
	}
	if (auto pPx = std::get_if<u32>(&size)) {
		ret.scale = (f32)(*pPx) / (f32)ret.maxBounds.y;
	} else {
		ret.scale = std::get<f32>(size);
	}
	ret.lineHeight = (f32)ret.maxBounds.y * ret.scale;
	ret.linePad = nPadY * ret.lineHeight;
	ret.textHeight = (f32)ret.lineHeight * ((f32)ret.lineCount + nPadY * f32(ret.lineCount - 1));
	return ret;
}
} // namespace le::res
