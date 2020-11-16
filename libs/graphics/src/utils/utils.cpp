#include <stb/stb_image.h>
#include <core/log.hpp>
#include <core/singleton.hpp>
#include <graphics/render_context.hpp>
#include <graphics/shader.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
struct Spv : Singleton<Spv> {
	Spv() {
		if (os::sysCall("{} --version", utils::g_compiler)) {
			bOnline = true;
			logD("[{}] SPIR-V compiler [{}] online", g_name, utils::g_compiler);
		} else {
			logW("[{}] Failed to bring SPIR-V compiler [{}] online", g_name, utils::g_compiler);
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
	qvi.offsets = {0, offsetof(Vertex, colour), offsetof(Vertex, normal), offsetof(Vertex, texCoord)};
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

std::optional<io::Path> utils::compileGlsl(io::Path const& src, io::Path dst, io::Path const& prefix, bool bDebug) {
	auto d = dst;
	if (d.empty()) {
		d = src;
		d += bDebug ? "-d.spv" : ".spv";
	}
	auto const flags = bDebug ? "-g" : std::string_view();
	auto const result = Spv::inst().compile(io::absolute(prefix / src), io::absolute(prefix / d), flags);
	if (!result.empty()) {
		logW("[{}] Failed to compile GLSL [{}] to SPIR-V: {}", g_name, src.generic_string(), result);
		return std::nullopt;
	}
	logD("[{}] Compiled GLSL [{}] to SPIR-V [{}]", g_name, src.generic_string(), d.generic_string());
	return d;
}

utils::SetBindings utils::extractBindings(Shader const& shader) {
	SetBindings ret;
	Sets sets;
	for (std::size_t idx = 0; idx < shader.m_spirV.size(); ++idx) {
		auto spirV = shader.m_spirV[idx];
		if (!spirV.empty()) {
			spvc::Compiler compiler(spirV);
			auto const resources = compiler.get_shader_resources();
			auto const type = Shader::typeToFlag[idx];
			extractBindings({compiler, resources}, sets, type);
			extractPush({compiler, resources}, ret.push, type);
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
	return ret;
}

RawImage utils::decompress(bytearray imgBytes, u8 channels) {
	RawImage ret;
	int ch;
	auto pIn = reinterpret_cast<stbi_uc const*>(imgBytes.data());
	auto pOut = stbi_load_from_memory(pIn, (int)imgBytes.size(), &ret.width, &ret.height, &ch, (int)channels);
	if (!pOut) {
		logW("[{}] Failed to decompress image data", g_name);
		return {};
	}
	std::size_t const size = (std::size_t)(ret.width * ret.height * channels);
	ret.bytes = Span(pOut, size);
	return ret;
}

void utils::release(RawImage rawImage) {
	if (!rawImage.bytes.empty()) {
		stbi_image_free((void*)rawImage.bytes.data());
	}
}
} // namespace le::graphics
