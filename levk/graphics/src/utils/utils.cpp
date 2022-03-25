#include <stb/stb_image.h>
#include <ktl/enumerate.hpp>
#include <ktl/stack_string.hpp>
#include <levk/core/log.hpp>
#include <levk/core/log_channel.hpp>
#include <levk/core/maths.hpp>
#include <levk/core/singleton.hpp>
#include <levk/core/utils/algo.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/core/utils/shell.hpp>
#include <levk/graphics/common.hpp>
#include <levk/graphics/render/context.hpp>
#include <levk/graphics/utils/instant_command.hpp>
#include <levk/graphics/utils/utils.hpp>

static_assert(sizeof(stbi_uc) == sizeof(std::byte) && alignof(stbi_uc) == alignof(std::byte), "Invalid type size/alignment");

namespace le::graphics {
namespace {
struct Spv : Singleton<Spv> {
	using Shell = le::utils::ShellSilent;

	Spv() {
		if (auto compiler = Shell(fmt::format("{} --version", utils::g_compiler).data())) {
			online = true;
			logI(LC_LibUser, "[{}] SPIR-V compiler online: {}", g_name, compiler.output());
		} else {
			logW(LC_LibUser, "[{}] Failed to bring SPIR-V compiler [{}] online: {}", g_name, utils::g_compiler, compiler.output());
		}
	}

	std::string compile(io::Path const& src, io::Path const& dst, std::string_view flags) {
		if (online) {
			if (!io::is_regular_file(src)) { return fmt::format("source file [{}] not found", src.generic_string()); }
			auto compile = Shell(fmt::format("{} {} {} -o {}", utils::g_compiler, flags, src.string(), dst.string()).data());
			if (!compile) { return fmt::format("compilation failed: {}", compile.output()); }
			return {};
		}
		return fmt::format("[{}] offline", utils::g_compiler);
	}

	bool online = false;
};
} // namespace

namespace {
struct DescBinding {
	std::string name;
	u32 count = 0;
	vk::DescriptorType type = vk::DescriptorType::eUniformBuffer;
	vk::ImageViewType imageType = vk::ImageViewType::e2D;
	vk::ShaderStageFlags stages;
	bool dummy = false;
};

namespace spvc = spirv_cross;
using binding_t = u32;
using bind_map = std::map<binding_t, DescBinding>;
using Sets = std::map<utils::set_t, bind_map>;
using Push = std::vector<vk::PushConstantRange>;
using Extractee = std::pair<spvc::Compiler const&, spvc::ShaderResources const&>;
using ExtractData = std::pair<vk::DescriptorType, vk::ShaderStageFlagBits>;
template <typename T>
using spvcSV = spvc::SmallVector<T>;

struct Extractor {
	spvc::Compiler const& c;
	Sets& m;

	void operator()(spvcSV<spvc::Resource> const& list, vk::DescriptorType type, vk::ShaderStageFlagBits stage) {
		for (auto const& item : list) {
			u32 const set = c.get_decoration(item.id, spv::Decoration::DecorationDescriptorSet);
			u32 const binding = c.get_decoration(item.id, spv::Decoration::DecorationBinding);
			auto& db = m[set][binding];
			db.type = type;
			db.stages |= stage;
			auto const& type = c.get_type(item.type_id);
			db.count = type.array.empty() ? 1 : type.array[0];
			db.name = item.name;
			if (type.basetype == spirv_cross::SPIRType::SampledImage || type.basetype == spirv_cross::SPIRType::Image) {
				switch (type.image.dim) {
				case spv::Dim::Dim1D: db.imageType = vk::ImageViewType::e1D; break;
				case spv::Dim::Dim2D:
				default: db.imageType = vk::ImageViewType::e2D; break;
				case spv::Dim::Dim3D: db.imageType = vk::ImageViewType::e3D; break;
				case spv::Dim::DimCube: db.imageType = vk::ImageViewType::eCube; break;
				}
			}
		}
	}
};

void extractBindings(Extractee ext, Sets& out_sets, vk::ShaderStageFlagBits stage) {
	auto const& [compiler, resources] = ext;
	Extractor e{compiler, out_sets};
	e(resources.uniform_buffers, vk::DescriptorType::eUniformBuffer, stage);
	e(resources.storage_buffers, vk::DescriptorType::eStorageBuffer, stage);
	e(resources.sampled_images, vk::DescriptorType::eCombinedImageSampler, stage);
	e(resources.separate_images, vk::DescriptorType::eSampledImage, stage);
	e(resources.storage_images, vk::DescriptorType::eStorageImage, stage);
	e(resources.separate_samplers, vk::DescriptorType::eSampler, stage);
}

void extractPush(Extractee ext, Push& out_push, vk::ShaderStageFlagBits stage) {
	auto const& [compiler, resources] = ext;
	u32 pcoffset = 0;
	for (auto const& pc : resources.push_constant_buffers) {
		vk::PushConstantRange pcr;
		pcr.stageFlags = stage;
		auto const& type = compiler.get_type(pc.base_type_id);
		pcr.size = (u32)compiler.get_declared_struct_size(type);
		pcr.offset = pcoffset;
		pcoffset += pcr.size;
		out_push.push_back(pcr);
	}
}
} // namespace

ktl::fixed_vector<ShaderResources, 4> utils::shaderResources(Span<SpirV> modules) {
	ktl::fixed_vector<ShaderResources, 4> ret;
	for (auto& module : modules) {
		if (!module.spirV.empty()) {
			ShaderResources res;
			res.compiler = std::make_unique<spvc::Compiler>(std::move(module.spirV));
			res.resources = res.compiler->get_shader_resources();
			res.type = module.type;
			ret.push_back(std::move(res));
		}
		EXPECT(ret.has_space());
		if (!ret.has_space()) { break; }
	}
	return ret;
}

io::Path utils::spirVpath(io::Path const& src, bool bDebug) {
	io::Path ret = src;
	ret += bDebug ? "-d.spv" : ".spv";
	return ret;
}

std::optional<io::Path> utils::compileGlsl(io::Path const& src, io::Path const& dst, io::Path const& prefix, bool bDebug) {
	auto const d = dst.empty() ? spirVpath(src, bDebug) : dst;
	auto const flags = bDebug ? "-g" : std::string_view();
	auto const result = Spv::inst().compile(io::absolute(prefix / src), io::absolute(prefix / d), flags);
	if (!result.empty()) {
		logW(LC_LibUser, "[{}] Failed to compile GLSL [{}] to SPIR-V: {}", g_name, src.generic_string(), result);
		return std::nullopt;
	}
	logI(LC_LibUser, "[{}] Compiled GLSL [{}] to SPIR-V [{}]", g_name, src.generic_string(), d.generic_string());
	return d;
}

utils::SetBindings utils::extractBindings(Span<SpirV> modules) {
	SetBindings ret;
	Sets sets;
	for (auto const& module : modules) {
		if (!module.spirV.empty()) {
			spvc::Compiler compiler(std::move(module.spirV));
			auto const resources = compiler.get_shader_resources();
			auto const stage = g_shaderStages[module.type];
			extractBindings({compiler, resources}, sets, stage);
			extractPush({compiler, resources}, ret.push, stage);
		}
	}
	for (u32 set = 0; set < (u32)sets.size(); ++set) {
		if (auto it = sets.find(set); it != sets.end()) {
			auto& bm = it->second;
			for (u32 binding = 0; binding < bm.size(); ++binding) {
				if (!bm.contains(binding)) {
					DescBinding dummy;
					dummy.dummy = true;
					bm[binding] = dummy; // inactive binding: no descriptors needed
				}
			}
		} else {
			sets[set] = bind_map(); // inactive set: has no bindings
		}
	}
	for (auto& [s, bmap] : sets) {
		auto& binds = ret.sets[s]; // register all set numbers whether active or not
		for (auto& [b, db] : bmap) {
			BindingInfo bindInfo;
			bindInfo.binding.binding = b;
			if (!db.dummy) {
				bindInfo.binding.stageFlags = db.stages;
				bindInfo.binding.descriptorCount = db.count;
				bindInfo.binding.descriptorType = db.type;
				bindInfo.name = std::move(db.name);
				bindInfo.imageType = db.imageType;
			} else {
				bindInfo.binding.descriptorCount = 0;
				bindInfo.name = fmt::format("[Unassigned_{}_{}]", s, b);
				bindInfo.binding.descriptorType = {};
			}
			ENSURE(binds.has_space(), "Max descriptor sets exceeded");
			binds.push_back(bindInfo);
		}
	}
	return ret;
}

namespace {
template <typename T, typename U>
void ensureSet(T& out_src, U&& u) {
	if (out_src == T()) { out_src = u; }
}

template <typename T, typename U>
void setIfSet(T& out_dst, U&& u) {
	if (!Device::default_v(u)) { out_dst = u; }
}

using RasterizerState = vk::PipelineRasterizationStateCreateInfo;
using BlendState = vk::PipelineColorBlendAttachmentState;
using DepthState = vk::PipelineDepthStencilStateCreateInfo;

void setStates(RasterizerState& rs, BlendState& bs, DepthState& ds, PFlags flags) {
	if (flags.test(PFlag::eDepthTest)) {
		ds.depthTestEnable = true;
		ds.depthCompareOp = vk::CompareOp::eLess;
	}
	if (flags.test(PFlag::eDepthWrite)) { ds.depthWriteEnable = true; }
	if (flags.test(PFlag::eAlphaBlend)) {
		using CCF = vk::ColorComponentFlagBits;
		bs.colorWriteMask = CCF::eR | CCF::eG | CCF::eB | CCF::eA;
		bs.blendEnable = true;
		bs.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		bs.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		bs.colorBlendOp = vk::BlendOp::eAdd;
		bs.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		bs.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		bs.alphaBlendOp = vk::BlendOp::eAdd;
	}
	if (flags.test(PFlag::eWireframe)) { rs.polygonMode = vk::PolygonMode::eLine; }
}
} // namespace

bool utils::hasActiveModule(Span<ShaderModule const> modules) noexcept {
	return !modules.empty() && std::any_of(std::begin(modules), std::end(modules), [](ShaderModule m) { return !Device::default_v(m.module); });
}

std::optional<vk::Pipeline> utils::makeGraphicsPipeline(Device& dv, Span<ShaderModule const> sm, PipelineSpec const& sp, PipeData const& data) {
	EXPECT(hasActiveModule(sm));
	EXPECT(!Device::default_v(data.renderPass) && !Device::default_v(data.layout));
	EXPECT(!sp.vertexInput.bindings.empty());
	if (Device::default_v(data.renderPass) || Device::default_v(data.layout) || !hasActiveModule(sm) || sp.vertexInput.bindings.empty()) {
		return std::nullopt;
	}
	vk::PipelineVertexInputStateCreateInfo vertexInputState;
	{
		auto const& vi = sp.vertexInput;
		vertexInputState.pVertexBindingDescriptions = vi.bindings.data();
		vertexInputState.vertexBindingDescriptionCount = (u32)vi.bindings.size();
		vertexInputState.pVertexAttributeDescriptions = vi.attributes.data();
		vertexInputState.vertexAttributeDescriptionCount = (u32)vi.attributes.size();
	}
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
	{
		inputAssemblyState.topology = sp.fixedState.topology;
		inputAssemblyState.primitiveRestartEnable = false;
	}
	vk::PipelineViewportStateCreateInfo viewportState;
	{
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
	}
	vk::PipelineRasterizationStateCreateInfo rasterizerState;
	{
		rasterizerState.depthClampEnable = false;
		rasterizerState.rasterizerDiscardEnable = false;
		rasterizerState.depthBiasEnable = false;
		auto const [lmin, lmax] = dv.lineWidthLimit();
		rasterizerState.lineWidth = std::clamp(sp.fixedState.lineWidth, lmin, lmax);
	}
	vk::PipelineColorBlendAttachmentState colorBlendState;
	{
		using CC = vk::ColorComponentFlagBits;
		colorBlendState.colorWriteMask = CC::eR | CC::eG | CC::eB | CC::eA;
		colorBlendState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	}
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	ensureSet(depthStencilState.depthCompareOp, vk::CompareOp::eLess);
	setStates(rasterizerState, colorBlendState, depthStencilState, sp.fixedState.flags);
	vk::PipelineColorBlendStateCreateInfo pcbsci;
	{
		pcbsci.logicOpEnable = false;
		pcbsci.attachmentCount = 1;
		pcbsci.pAttachments = &colorBlendState;
	}
	vk::PipelineDynamicStateCreateInfo dynamicState;
	{
		dynamicState.dynamicStateCount = (u32)sp.fixedState.dynamicStates.size();
		dynamicState.pDynamicStates = sp.fixedState.dynamicStates.data();
	}
	ktl::fixed_vector<vk::PipelineShaderStageCreateInfo, 8> shaderCreateInfo;
	{
		static constexpr EnumArray<ShaderType, vk::ShaderStageFlagBits> flag = {
			vk::ShaderStageFlagBits::eVertex,
			vk::ShaderStageFlagBits::eFragment,
			vk::ShaderStageFlagBits::eCompute,
		};
		for (auto const module : sm) {
			vk::PipelineShaderStageCreateInfo createInfo;
			createInfo.stage = flag[module.type];
			createInfo.module = module.module;
			createInfo.pName = "main";
			shaderCreateInfo.push_back(std::move(createInfo));
		}
	}
	vk::PipelineMultisampleStateCreateInfo multisamplerState;
	vk::GraphicsPipelineCreateInfo createInfo;
	createInfo.stageCount = (u32)shaderCreateInfo.size();
	createInfo.pStages = shaderCreateInfo.data();
	createInfo.pVertexInputState = &vertexInputState;
	createInfo.pInputAssemblyState = &inputAssemblyState;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterizerState;
	createInfo.pMultisampleState = &multisamplerState;
	createInfo.pDepthStencilState = &depthStencilState;
	createInfo.pColorBlendState = &pcbsci;
	createInfo.pDynamicState = &dynamicState;
	createInfo.layout = data.layout;
	createInfo.renderPass = data.renderPass;
	createInfo.subpass = sp.subpass;
	auto ret = dv.device().createGraphicsPipeline(data.cache, createInfo);
	if (ret.result == vk::Result::eSuccess) { return ret.value; }
	return std::nullopt;
}

Bitmap utils::bitmap(std::initializer_list<Colour> pixels, u32 width, u32 height) { return bitmap(Span(&*pixels.begin(), pixels.size()), width, height); }

Bitmap utils::bitmap(Span<Colour const> pixels, u32 width, u32 height) {
	Bitmap ret{{}, {width, height > 0 ? height : u32(pixels.size()) / width}};
	ret.bytes.reserve(pixels.size() * 4);
	for (Colour const pixel : pixels) { append(ret.bytes, pixel); }
	return ret;
}

void utils::append(BmpBytes& out, Colour pixel) {
	u8 const bytes[] = {pixel.r.value, pixel.g.value, pixel.b.value, pixel.a.value};
	out.reserve(out.size() + arraySize(bytes));
	for (auto const byte : bytes) { out.push_back(byte); }
}

utils::STBImg::STBImg(ImageData compressed, u8 channels) {
	ENSURE(compressed.size() <= (std::size_t)maths::max<int>(), "size too large!");
	auto pIn = reinterpret_cast<stbi_uc const*>(compressed.data());
	int w, h, ch;
	auto pOut = stbi_load_from_memory(pIn, (int)compressed.size(), &w, &h, &ch, (int)channels);
	if (!pOut) { logW(LC_EndUser, "[{}] Failed to decompress image data", g_name); }
	extent = {u32(w), u32(h)};
	bytes = Span(pOut, std::size_t(extent.x * extent.y * channels));
}

utils::STBImg::~STBImg() {
	if (!bytes.empty()) { stbi_image_free((void*)bytes.data()); }
}

void utils::STBImg::exchg(STBImg& lhs, STBImg& rhs) noexcept {
	std::swap(static_cast<TBitmap<Span<u8 const>>&>(lhs), static_cast<TBitmap<Span<u8 const>>&>(rhs));
}

std::array<bytearray, 6> utils::loadCubemap(io::Media const& media, io::Path const& prefix, std::string_view ext, CubeImageURIs const& ids) {
	std::array<bytearray, 6> ret;
	for (auto const& [id, idx] : ktl::enumerate(ids)) {
		io::Path const name = io::Path(id) + ext;
		io::Path const path = prefix / name;
		if (auto bytes = media.bytes(path)) {
			ret[idx] = std::move(*bytes);
		} else {
			logW(LC_EndUser, "[{}] Failed to load bytes from [{}]", g_name, path.generic_string());
		}
	}
	return ret;
}

void utils::Transition::operator()(vk::ImageLayout layout, LayerMip const& lm, LayoutStages const& ls) const {
	if (layout != vIL::eUndefined) { device->m_layouts.transition(cb->m_cb, image, layout, ls, lm); }
}

BlitFlags utils::blitFlags(not_null<Device*> device, ImageRef const& img) {
	auto const& bcs = device->physicalDevice().blitCaps(img.format);
	return img.linear ? bcs.linear : bcs.optimal;
}

bool utils::canBlit(not_null<Device*> device, TPair<ImageRef> const& images, BlitFilter filter) {
	auto const& bfs = blitFlags(device, images.first);
	auto const& bfd = blitFlags(device, images.second);
	if (filter == BlitFilter::eLinear) {
		return bfs.all(BlitFlags(BlitFlag::eSrc, BlitFlag::eLinearFilter)) && bfd.all(BlitFlags(BlitFlag::eDst, BlitFlag::eLinearFilter));
	} else {
		return bfs.test(BlitFlag::eSrc) && bfs.test(BlitFlag::eDst);
	}
}

namespace {
template <typename F>
auto xferImage(not_null<VRAM*> vram, CommandBuffer cb, ImageRef const& src, Image const& out_dst, F func) {
	LayoutPair const layouts = {vram->m_device->m_layouts.get(src.image), vram->m_device->m_layouts.get(out_dst.image())};
	EXPECT(layouts.first != vIL::eUndefined);
	utils::Transition tsrc{vram->m_device, &cb, src.image};
	utils::Transition tdst{vram->m_device, &cb, out_dst.image()};
	tsrc(vIL::eTransferSrcOptimal);
	tdst(vIL::eTransferDstOptimal);
	auto const ret = func();
	tsrc(layouts.first);
	if (layouts.second != vIL::eUndefined) { tdst(layouts.second); }
	if (out_dst.mipCount() > 1U) { vram->makeMipMaps(cb, out_dst, {layouts.second, layouts.second}); }
	return ret;
}
} // namespace

bool utils::blit(not_null<VRAM*> vram, CommandBuffer cb, ImageRef const& src, Image const& out_dst, BlitFilter filter) {
	TPair<ImageRef> const imgs = {src, out_dst.ref()};
	bool const blittable = canBlit(vram->m_device, imgs, filter);
	EXPECT(blittable);
	if (!blittable) { return false; }
	return xferImage(vram, cb, src, out_dst, [vram, cb, imgs, filter] { return vram->blit(cb, imgs, filter); });
}

bool utils::copy(not_null<VRAM*> vram, CommandBuffer cb, ImageRef const& src, Image const& out_dst) {
	TPair<ImageRef> const imgs = {src, out_dst.ref()};
	return xferImage(vram, cb, src, out_dst, [vram, cb, imgs] { return vram->copy(cb, imgs); });
}

bool utils::blitOrCopy(not_null<VRAM*> vram, CommandBuffer cb, ImageRef const& src, Image const& out_dst, BlitFilter filter) {
	TPair<ImageRef> const imgs = {src, out_dst.ref()};
	return canBlit(vram->m_device, imgs, filter) ? blit(vram, cb, src, out_dst, filter) : copy(vram, cb, src, out_dst);
}

Memory::UniqueResource utils::copySub(not_null<VRAM*> vram, CommandBuffer cb, Bitmap const& bitmap, Image const& out_dst, glm::ivec2 ioffset) {
	auto const extent = vk::Extent3D(bitmap.extent.x, bitmap.extent.y, 1U);
	auto copyRegion = VRAM::bufferImageCopy(extent, vk::ImageAspectFlagBits::eColor, 0U, ioffset, 0U);
	auto const layout = vram->m_device->m_layouts.get(out_dst.image());
	auto buffer = vram->makeStagingBuffer(bitmap.bytes.size());
	void const* data = buffer.map();
	std::memcpy((u8*)data, bitmap.bytes.data(), bitmap.bytes.size());
	VRAM::ImgMeta meta;
	meta.layouts = {layout, layout};
	meta.stages = {vPSFB::eAllCommands, vPSFB::eAllCommands};
	meta.access = {vAFB::eMemoryRead | vAFB::eMemoryWrite, vAFB::eMemoryRead | vAFB::eMemoryWrite};
	VRAM::copy(cb.m_cb, buffer.buffer(), out_dst.image(), copyRegion, meta);
	if (out_dst.mipCount() > 1U) { vram->makeMipMaps(cb, out_dst, meta.layouts); }
	return std::move(buffer).resource();
}

std::optional<Image> utils::makeStorage(not_null<VRAM*> vram, ImageRef const& src) {
	if (!src.image) { return std::nullopt; }
	ImageRef dst = src;
	dst.format = vk::Format::eR8G8B8A8Unorm;
	dst.linear = true;
	// if image will be copied, match RGBA vs BGRA to source format
	if (!canBlit(vram->m_device, {src, dst}) && Surface::bgra(src.format)) { dst.format = vk::Format::eB8G8R8A8Unorm; }
	Image ret(vram, Image::storageInfo(dst.extent, dst.format));
	auto cmd = BlockingCommand(vram);
	if (blitOrCopy(vram, cmd.cb(), src, ret)) {
		ret.map();
		return ret;
	}
	return std::nullopt;
}

std::size_t utils::writePPM(not_null<Device*> device, Image const& img, std::ostream& out_str) {
	EXPECT(img.layerCount() == 1U && img.mapped());
	std::size_t ret{};
	u8 const* data = (u8 const*)img.mapped();
	if (img.layerCount() == 1U && data) {
		bool const swizzle = Surface::bgra(img.format());
		auto const extent = img.extent2D();
		auto isr = device->device().getImageSubresourceLayout(img.image(), vk::ImageSubresource(vIAFB::eColor));
		if (isr.offset < extent.x * extent.y * 4U) { data += isr.offset; }
		auto const header = ktl::stack_string<256>("P6\n{}\n{}\n255\n", extent.x, extent.y);
		out_str << header.get();
		for (u32 y = 0; y < extent.y; ++y) {
			auto row = (u32 const*)data;
			for (u32 x = 0; x < extent.x; ++x) {
				auto const ch = (u8 const*)row;
				out_str << (swizzle ? ch[2] : ch[0]);
				out_str << ch[1];
				out_str << (swizzle ? ch[0] : ch[2]);
				ret += 3;
				++row;
			}
			data += isr.rowPitch;
		}
	}
	return ret;
}

utils::HashGen& utils::operator<<(HashGen& out, VertexInputInfo const& vi) {
	out << vi.bindings.size() << vi.attributes.size();
	for (auto const& bind : vi.bindings) { out << bind.binding << bind.stride << bind.inputRate; }
	for (auto const& attr : vi.attributes) { out << attr.binding << attr.location << attr.offset << attr.format; }
	return out;
}

utils::HashGen& utils::operator<<(HashGen& out, ShaderSpec const& ss) {
	for (Hash const uri : ss.moduleURIs) { out << uri; }
	return out;
}

utils::HashGen& utils::operator<<(HashGen& out, FixedStateSpec const& fs) {
	out << fs.dynamicStates.size();
	for (auto const state : fs.dynamicStates) { out << state; }
	return out << fs.flags.bits() << fs.mode << fs.topology << fs.lineWidth;
}

utils::HashGen& utils::operator<<(HashGen& out, PipelineSpec const& ps) { return out << ps.vertexInput << ps.fixedState << ps.shader << ps.subpass; }
} // namespace le::graphics
