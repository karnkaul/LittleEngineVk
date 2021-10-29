#include <stb/stb_image.h>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <core/singleton.hpp>
#include <core/utils/algo.hpp>
#include <core/utils/shell.hpp>
#include <graphics/common.hpp>
#include <graphics/render/context.hpp>
#include <graphics/render/pipeline.hpp>
#include <graphics/shader.hpp>
#include <graphics/utils/utils.hpp>

static_assert(sizeof(stbi_uc) == sizeof(std::byte) && alignof(stbi_uc) == alignof(std::byte), "Invalid type size/alignment");

namespace le::graphics {
namespace {
struct Spv : Singleton<Spv> {
	using Shell = le::utils::ShellSilent;

	Spv() {
		if (auto compiler = Shell(fmt::format("{} --version", utils::g_compiler).data())) {
			online = true;
			g_log.log(lvl::info, 1, "[{}] SPIR-V compiler online: {}", g_name, compiler.output());
		} else {
			g_log.log(lvl::warn, 1, "[{}] Failed to bring SPIR-V compiler [{}] online: {}", g_name, utils::g_compiler, compiler.output());
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

VertexInputInfo VertexInfoFactory<Vertex>::operator()(u32 binding) const {
	QuickVertexInput qvi;
	qvi.binding = binding;
	qvi.size = sizeof(Vertex);
	qvi.attributes = {{vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
					  {vk::Format::eR32G32B32Sfloat, offsetof(Vertex, colour)},
					  {vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
					  {vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)}};
	return RenderContext::vertexInput(qvi);
}

Shader::ResourcesMap utils::shaderResources(Shader const& shader) {
	Shader::ResourcesMap ret;
	for (std::size_t idx = 0; idx < arraySize(shader.m_spirV.arr); ++idx) {
		auto spirV = shader.m_spirV.arr[idx];
		if (!spirV.empty()) {
			auto& res = ret.arr[idx];
			res.compiler = std::make_unique<spvc::Compiler>(spirV);
			res.resources = res.compiler->get_shader_resources();
		}
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
		g_log.log(lvl::warn, 1, "[{}] Failed to compile GLSL [{}] to SPIR-V: {}", g_name, src.generic_string(), result);
		return std::nullopt;
	}
	g_log.log(lvl::info, 1, "[{}] Compiled GLSL [{}] to SPIR-V [{}]", g_name, src.generic_string(), d.generic_string());
	return d;
}

utils::SetBindings utils::extractBindings(Shader const& shader) {
	SetBindings ret;
	Sets sets;
	for (std::size_t idx = 0; idx < arraySize(shader.m_spirV.arr); ++idx) {
		auto spirV = shader.m_spirV.arr[idx];
		if (!spirV.empty()) {
			spvc::Compiler compiler(spirV);
			auto const resources = compiler.get_shader_resources();
			auto const type = Shader::typeToFlag[idx];
			extractBindings({compiler, resources}, sets, type);
			extractPush({compiler, resources}, ret.push, type);
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
			} else {
				bindInfo.binding.descriptorCount = 0;
				bindInfo.name = fmt::format("[Unassigned_{}_{}]", s, b);
				bindInfo.bUnassigned = true;
			}
			ensure(binds.has_space(), "Max descriptor sets exceeded");
			binds.push_back(bindInfo);
		}
	}
	return ret;
}

Bitmap utils::bitmap(std::initializer_list<Colour> pixels, u32 width, u32 height) { return bitmap(Span(&*pixels.begin(), pixels.size()), width, height); }

Bitmap utils::bitmap(Span<Colour const> pixels, u32 width, u32 height) {
	Bitmap ret{{}, {width, height > 0 ? height : u32(pixels.size()) / width}, false};
	ret.bytes.reserve(pixels.size() * 4);
	for (Colour const pixel : pixels) { append(ret.bytes, pixel); }
	return ret;
}

void utils::append(BmpBytes& out, Span<std::byte const> bytes) {
	out.reserve(out.size() + bytes.size());
	for (auto const byte : bytes) { out.push_back(static_cast<Bitmap::type::value_type>(byte)); }
}

void utils::append(BmpBytes& out, Colour pixel) {
	u8 const bytes[] = {pixel.r.value, pixel.g.value, pixel.b.value, pixel.a.value};
	out.reserve(out.size() + arraySize(bytes));
	for (auto const byte : bytes) { out.push_back(byte); }
}

BmpBytes utils::bmpBytes(Span<std::byte const> bytes) {
	BmpBytes ret;
	append(ret, bytes);
	return ret;
}

utils::STBImg::STBImg(Bitmap::type const& compressed, u8 channels) {
	ensure(compressed.size() <= (std::size_t)maths::max<int>(), "size too large!");
	auto pIn = reinterpret_cast<stbi_uc const*>(compressed.data());
	int w, h, ch;
	auto pOut = stbi_load_from_memory(pIn, (int)compressed.size(), &w, &h, &ch, (int)channels);
	if (!pOut) { g_log.log(lvl::warn, 1, "[{}] Failed to decompress image data", g_name); }
	size = {u32(w), u32(h)};
	bytes = Span(pOut, std::size_t(size.x * size.y * channels));
}

utils::STBImg::~STBImg() {
	if (!bytes.empty()) { stbi_image_free(bytes.data()); }
}

void utils::STBImg::exchg(STBImg& lhs, STBImg& rhs) noexcept { std::swap(static_cast<TBitmap<Span<u8>>&>(lhs), static_cast<TBitmap<Span<u8>>&>(rhs)); }

std::array<bytearray, 6> utils::loadCubemap(io::Media const& media, io::Path const& prefix, std::string_view ext, CubeImageIDs const& ids) {
	std::array<bytearray, 6> ret;
	std::size_t idx = 0;
	for (std::string_view id : ids) {
		io::Path const name = io::Path(id) + ext;
		io::Path const path = prefix / name;
		if (auto bytes = media.bytes(path)) {
			ret[idx++] = std::move(*bytes);
		} else {
			g_log.log(lvl::warn, 0, "[{}] Failed to load bytes from [{}]", g_name, path.generic_string());
		}
	}
	return ret;
}

std::vector<QueueMultiplex::Family> utils::queueFamilies(PhysicalDevice const& device, vk::SurfaceKHR surface) {
	using vkqf = vk::QueueFlagBits;
	std::vector<QueueMultiplex::Family> ret;
	u32 fidx = 0;
	for (vk::QueueFamilyProperties const& props : device.queueFamilies) {
		QueueMultiplex::Family family;
		family.familyIndex = fidx;
		family.total = props.queueCount;
		bool const bSurfaceSupport = device.device.getSurfaceSupportKHR(fidx, surface);
		if ((props.queueFlags & vkqf::eTransfer) == vkqf::eTransfer) { family.flags.set(QType::eTransfer); }
		if ((props.queueFlags & vkqf::eGraphics) == vkqf::eGraphics) { family.flags.set(QFlags(QType::eGraphics) | QType::eTransfer); }
		if (bSurfaceSupport) { family.flags.set(QType::ePresent); }
		ret.push_back(family);
		++fidx;
	}
	return ret;
}
} // namespace le::graphics
