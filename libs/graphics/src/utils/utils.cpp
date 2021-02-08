#include <stb/stb_image.h>
#include <core/log.hpp>
#include <core/singleton.hpp>
#include <graphics/common.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/render_context.hpp>
#include <graphics/shader.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
struct Spv : Singleton<Spv> {
	Spv() {
		if (os::sysCall("{} --version", utils::g_compiler)) {
			bOnline = true;
			g_log.log(lvl::info, 1, "[{}] SPIR-V compiler [{}] online", g_name, utils::g_compiler);
		} else {
			g_log.log(lvl::warning, 1, "[{}] Failed to bring SPIR-V compiler [{}] online", g_name, utils::g_compiler);
		}
	}

	std::string compile(io::Path const& src, io::Path const& dst, std::string_view flags) {
		if (bOnline) {
			if (!io::is_regular_file(src)) {
				return fmt::format("source file [{}] not found", src.generic_string());
			}
			if (!os::sysCall("{} {} {} -o {}", utils::g_compiler, flags, src.string(), dst.string())) {
				return "compilation failed";
			}
			return {};
		}
		return fmt::format("[{}] offline", utils::g_compiler);
	}

	bool bOnline = false;
};
} // namespace

namespace {
struct DescBinding {
	std::string name;
	u32 count = 0;
	vk::DescriptorType type = vk::DescriptorType::eUniformBuffer;
	vk::ShaderStageFlags stages;
};

namespace spvc = spirv_cross;
using binding_t = u32;
using Sets = std::map<utils::set_t, std::map<binding_t, DescBinding>>;
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
			DescBinding& db = m[set][binding];
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
	for (std::size_t idx = 0; idx < shader.m_spirV.size(); ++idx) {
		auto spirV = shader.m_spirV[idx];
		if (!spirV.empty()) {
			auto& res = ret[idx];
			res.compiler = std::make_unique<spvc::Compiler>(spirV);
			res.resources = ret[idx].compiler->get_shader_resources();
		}
	}
	return ret;
}

io::Path utils::spirVpath(io::Path const& src, bool bDebug) {
	io::Path ret = src;
	ret += bDebug ? "-d.spv" : ".spv";
	return ret;
}

kt::result_t<io::Path> utils::compileGlsl(io::Path const& src, io::Path const& dst, io::Path const& prefix, bool bDebug) {
	auto const d = dst.empty() ? spirVpath(src, bDebug) : dst;
	auto const flags = bDebug ? "-g" : std::string_view();
	auto const result = Spv::inst().compile(io::absolute(prefix / src), io::absolute(prefix / d), flags);
	if (!result.empty()) {
		g_log.log(lvl::warning, 1, "[{}] Failed to compile GLSL [{}] to SPIR-V: {}", g_name, src.generic_string(), result);
		return {};
	}
	g_log.log(lvl::info, 1, "[{}] Compiled GLSL [{}] to SPIR-V [{}]", g_name, src.generic_string(), d.generic_string());
	return d;
}

utils::SetBindings utils::extractBindings(Shader const& shader) {
	SetBindings ret;
	Sets sets;
	std::vector<u32> gaps;
	for (std::size_t idx = 0; idx < shader.m_spirV.size(); ++idx) {
		auto spirV = shader.m_spirV[idx];
		if (!spirV.empty()) {
			spvc::Compiler compiler(spirV);
			auto const resources = compiler.get_shader_resources();
			auto const type = Shader::typeToFlag[idx];
			extractBindings({compiler, resources}, sets, type);
			extractPush({compiler, resources}, ret.push, type);
		}
		if (!sets.empty()) {
			u32 set = 0;
			for (auto& [s, _] : sets) {
				if (s == set) {
					++set;
					continue;
				}
				while (s > set) {
					gaps.push_back(set++);
				}
			}
		}
	}
	for (auto const& [s, bmap] : sets) {
		for (auto const& [b, db] : bmap) {
			BindingInfo bindInfo;
			bindInfo.binding.binding = b;
			bindInfo.binding.stageFlags = db.stages;
			bindInfo.binding.descriptorCount = db.count;
			bindInfo.binding.descriptorType = db.type;
			bindInfo.name = std::move(db.name);
			ret.sets[s].push_back(bindInfo);
		}
	}
	for (auto set : gaps) {
		BindingInfo binding;
		binding.binding.descriptorCount = 0;
		binding.name = fmt::format("[Unassigned_{}]", set);
		binding.bUnassigned = true;
		ret.sets[set].push_back(binding);
	}
	return ret;
}

bytearray utils::convert(std::initializer_list<u8> bytes) {
	return bytes.size() == 0 ? bytearray() : convert(View<u8>(&(*bytes.begin()), bytes.size()));
}

bytearray utils::convert(View<u8> bytes) {
	bytearray ret;
	for (u8 byte : bytes) {
		ret.push_back(static_cast<std::byte>(byte));
	}
	return ret;
}

Texture::RawImage utils::decompress(bytearray imgBytes, u8 channels) {
	Texture::RawImage ret;
	int ch;
	auto pIn = reinterpret_cast<stbi_uc const*>(imgBytes.data());
	auto pOut = stbi_load_from_memory(pIn, (int)imgBytes.size(), &ret.width, &ret.height, &ch, (int)channels);
	if (!pOut) {
		g_log.log(lvl::warning, 1, "[{}] Failed to decompress image data", g_name);
		return {};
	}
	std::size_t const size = (std::size_t)(ret.width * ret.height * channels);
	ret.bytes = View<std::byte>(reinterpret_cast<std::byte*>(pOut), size);
	return ret;
}

void utils::release(Texture::RawImage rawImage) {
	if (!rawImage.bytes.empty()) {
		stbi_image_free((void*)rawImage.bytes.data());
	}
}

std::array<bytearray, 6> utils::loadCubemap(io::Reader const& reader, io::Path const& prefix, std::string_view ext, CubeImageIDs const& ids) {
	std::array<bytearray, 6> ret;
	std::size_t idx = 0;
	for (std::string_view id : ids) {
		io::Path const name = io::Path(id) + ext;
		io::Path const path = prefix / name;
		if (auto bytes = reader.bytes(path)) {
			ret[idx++] = std::move(*bytes);
		} else {
			g_log.log(lvl::warning, 0, "[{}] Failed to load bytes from [{}]", g_name, path.generic_string());
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
		if ((props.queueFlags & vkqf::eTransfer) == vkqf::eTransfer) {
			family.flags.set(QType::eTransfer);
		}
		if ((props.queueFlags & vkqf::eGraphics) == vkqf::eGraphics) {
			family.flags.set(QType::eGraphics | QType::eTransfer);
		}
		if (bSurfaceSupport) {
			family.flags.set(QType::ePresent);
		}
		ret.push_back(family);
		++fidx;
	}
	return ret;
}
} // namespace le::graphics
