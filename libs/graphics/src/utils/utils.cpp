#include <stb/stb_image.h>
#include <core/log.hpp>
#include <core/log_channel.hpp>
#include <core/maths.hpp>
#include <core/singleton.hpp>
#include <core/utils/algo.hpp>
#include <core/utils/expect.hpp>
#include <core/utils/shell.hpp>
#include <graphics/common.hpp>
#include <graphics/render/context.hpp>
#include <graphics/utils/command_rotator.hpp>
#include <graphics/utils/utils.hpp>
#include <ktl/stack_string.hpp>

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
	bool bDummy = false;
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
				if (!le::utils::contains(bm, binding)) {
					DescBinding dummy;
					dummy.bDummy = true;
					bm[binding] = dummy; // inactive binding: no descriptors needed
				}
			}
		} else {
			sets[set] = bind_map(); // inactive set: has no bindings
		}
	}
	for (auto const& [s, bmap] : sets) {
		auto& binds = ret.sets[s]; // register all set numbers whether active or not
		for (auto const& [b, db] : bmap) {
			BindingInfo bindInfo;
			bindInfo.binding.binding = b;
			if (!db.bDummy) {
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
	std::size_t idx = 0;
	for (std::string_view id : ids) {
		io::Path const name = io::Path(id) + ext;
		io::Path const path = prefix / name;
		if (auto bytes = media.bytes(path)) {
			ret[idx++] = std::move(*bytes);
		} else {
			logW(LC_EndUser, "[{}] Failed to load bytes from [{}]", g_name, path.generic_string());
		}
	}
	return ret;
}

void utils::Transition::operator()(vk::ImageLayout layout, LayoutStages const& stages) const {
	if (layout != vIL::eUndefined) { device->m_layouts.transition(*cb, image, layout, stages); }
}

utils::DualTransition::DualTransition(not_null<Device*> device, not_null<CommandBuffer*> cb, TPair<vk::Image> images, LayoutPair layouts, Stages const& stages)
	: m_a{device, cb, images.first}, m_b{device, cb, images.second}, m_stages(stages.second) {
	m_layouts = {device->m_layouts.get(images.first), device->m_layouts.get(images.second)};
	m_a(layouts.first, stages.first);
	m_b(layouts.second, stages.first);
}

utils::DualTransition::~DualTransition() {
	m_a(m_layouts.first, m_stages);
	m_b(m_layouts.second, m_stages);
}

std::vector<QueueMultiplex::Family> utils::queueFamilies(PhysicalDevice const& device, vk::SurfaceKHR surface) {
	using vkqf = vk::QueueFlagBits;
	std::vector<QueueMultiplex::Family> ret;
	u32 fidx = 0;
	for (vk::QueueFamilyProperties const& props : device.queueFamilies) {
		QueueMultiplex::Family family;
		family.familyIndex = fidx;
		family.total = props.queueCount;
		if ((props.queueFlags & vkqf::eTransfer) == vkqf::eTransfer) { family.flags.set(QType::eTransfer); }
		if ((props.queueFlags & vkqf::eGraphics) == vkqf::eGraphics) { family.flags.set(QFlags(QType::eGraphics) | QType::eTransfer); }
		if (device.device.getSurfaceSupportKHR(fidx, surface)) { family.flags.set(QType::ePresent); }
		ret.push_back(family);
		++fidx;
	}
	return ret;
}

bool utils::blit(not_null<VRAM*> vram, CommandBuffer cb, Image const& src, Image& out_dst, vk::Filter filter) {
	EXPECT(src.layerCount() == 1U && out_dst.layerCount() == 1U);
	EXPECT(canBlit(src, out_dst));
	if (src.layerCount() > 1U || out_dst.layerCount() > 1U || !canBlit(src, out_dst)) { return false; }
	DualTransition dt(vram->m_device, &cb, {src.image(), out_dst.image()});
	return vram->blit(cb, src, out_dst, filter);
}

bool utils::copy(not_null<VRAM*> vram, CommandBuffer cb, Image const& src, Image& out_dst) {
	EXPECT(src.layerCount() == 1U && out_dst.layerCount() == 1U);
	EXPECT(src.extent() == out_dst.extent());
	if (src.layerCount() > 1U || out_dst.layerCount() > 1U) { return false; }
	if (src.extent() != out_dst.extent()) { return false; }
	DualTransition dt(vram->m_device, &cb, {src.image(), out_dst.image()});
	return vram->copy(cb, src, out_dst);
}

bool utils::blitOrCopy(not_null<VRAM*> vram, CommandBuffer cb, Image const& src, Image& out_dst, vk::Filter filter) {
	return canBlit(src, out_dst) ? blit(vram, cb, src, out_dst, filter) : copy(vram, cb, src, out_dst);
}

std::optional<Image> utils::makeStorage(not_null<VRAM*> vram, CommandRotator const& cr, Image const& img) {
	vk::Format format = Image::linear_v;
	bool const blittable = img.blitFlags().test(BlitFlag::eSrc) && vram->m_device->physicalDevice().blitCaps(format).linear.test(BlitFlag::eDst);
	if (!blittable && Surface::bgra(img.imageFormat())) { format = vk::Format::eB8G8R8A8Unorm; }
	Image ret(vram, Image::storageInfo(img.extent2D(), format));
	if (auto cmd = cr.instant()) {
		if (blitOrCopy(vram, cmd.cb(), img, ret)) { return ret; }
	}
	return std::nullopt;
}

std::size_t utils::writePPM(not_null<Device*> device, Image& out_img, std::ostream& out_str) {
	EXPECT(out_img.layerCount() == 1U);
	std::size_t ret{};
	if (out_img.layerCount() == 1U) {
		bool const swizzle = Surface::bgra(out_img.imageFormat());
		auto const extent = out_img.extent2D();
		auto isr = device->device().getImageSubresourceLayout(out_img.image(), vk::ImageSubresource(vIAFB::eColor));
		u8 const* data = (u8 const*)out_img.map();
		data += isr.offset;
		auto const header = ktl::stack_string<256>("P6\n%u\n%u\n255\n", extent.x, extent.y);
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
