#include <core/utils/expect.hpp>
#include <graphics/render/context.hpp>
#include <graphics/render/pipeline_factory.hpp>
#include <graphics/utils/utils.hpp>

#include <core/services.hpp>

namespace le::graphics {
std::size_t PipelineFactory::Hasher::operator()(Spec const& spec) const {
	utils::HashGen ret;
	return ret << spec.vertexInput << spec.fixedState << spec.shader;
}

PipelineFactory::Spec PipelineFactory::spec(ShaderSpec shader, PFlags flags, VertexInputInfo vertexInput) {
	Spec ret;
	EXPECT(!shader.modules.empty());
	ret.shader = shader;
	ret.fixedState.flags = flags;
	ret.vertexInput = vertexInput.bindings.empty() ? VertexInfoFactory<Vertex>{}(0) : std::move(vertexInput);
	return ret;
}

vk::UniqueShaderModule PipelineFactory::makeModule(vk::Device device, SpirV const& spirV) {
	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = spirV.size() * sizeof(SpirV::value_type);
	createInfo.pCode = spirV.data();
	return device.createShaderModuleUnique(createInfo);
}

PipelineFactory::PipelineFactory(not_null<VRAM*> vram, GetSpirV&& getSpirV, Buffering buffering) noexcept
	: m_getSpirV(std::move(getSpirV)), m_vram(vram), m_buffering(buffering) {
	EXPECT(m_getSpirV.has_value());
}

Pipeline PipelineFactory::get(Spec const& spec, vk::RenderPass renderPass) {
	EXPECT(!spec.shader.modules.empty());
	EXPECT(!Device::default_v(renderPass));
	EXPECT(!spec.vertexInput.bindings.empty() && !spec.vertexInput.attributes.empty());
	auto const specHash = spec.hash();
	auto sit = m_storage.find(specHash);
	if (sit == m_storage.end()) {
		auto [i, b] = m_storage.emplace(specHash, SpecMap());
		sit = i;
		sit->second.spec = spec;
	}
	auto& specMap = sit->second;
	if (!specMap.meta.layout.active()) { specMap.meta = makeMeta(spec.shader); }
	auto pit = specMap.map.find(renderPass);
	if (pit == specMap.map.end() || pit->second.stale) {
		auto pipe = makePipe(specMap, renderPass);
		ENSURE(pipe, "Failed to create pipeline");
		auto [i, b] = specMap.map.insert_or_assign(renderPass, std::move(*pipe));
		pit = i;
	}
	return pit->second.pipe();
}

bool PipelineFactory::contains(Hash spec, vk::RenderPass renderPass) const {
	if (auto sit = m_storage.find(spec); sit != m_storage.end()) { return sit->second.map.contains(renderPass); }
	return false;
}

PipelineSpec const* PipelineFactory::find(Hash spec) const {
	if (auto sit = m_storage.find(spec); sit != m_storage.end()) { return &sit->second.spec; }
	return nullptr;
}

std::size_t PipelineFactory::markStale(Hash shaderURI) {
	std::size_t ret{};
	for (auto& [_, specMap] : m_storage) {
		for (auto const& mod : specMap.spec.shader.modules) {
			if (mod.uri == shaderURI) {
				specMap.map.clear();						  // destroy pipelines
				specMap.meta = makeMeta(specMap.spec.shader); // recreate shader
				++ret;
				break;
			}
		}
	}
	return ret;
}

std::size_t PipelineFactory::pipeCount(Hash spec) const noexcept {
	if (auto sit = m_storage.find(spec); sit != m_storage.end()) { return sit->second.map.size(); }
	return {};
}

void PipelineFactory::clear(Hash spec) noexcept {
	if (auto sit = m_storage.find(spec); sit != m_storage.end()) { sit->second.map.clear(); }
}

std::optional<PipelineFactory::Pipe> PipelineFactory::makePipe(SpecMap const& spec, vk::RenderPass renderPass) const {
	// auto const& shader = m_getShader(spec.spec.shaderURI);
	// EXPECT(utils::hasActiveModule(shader.m_modules));
	// if (!utils::hasActiveModule(shader.m_modules)) { return std::nullopt; }
	Pipe ret;
	ret.layout = spec.meta.layout;
	utils::PipeData data;
	data.renderPass = renderPass;
	data.layout = spec.meta.layout;
	ktl::fixed_vector<vk::UniqueShaderModule, 4> modules;
	ShaderSpec::ModMap modMap;
	for (auto const module : spec.spec.shader.modules) {
		modules.push_back(makeModule(m_vram->m_device->device(), m_getSpirV(module.uri)));
		modMap[module.type] = *modules.back();
	}
	// TODO
	// in.cache = m_pipelineCache;
	if (auto p = utils::makeGraphicsPipeline(*m_vram->m_device, modMap, spec.spec, data)) {
		ret.pipeline = Deferred<vk::Pipeline>{m_vram->m_device, *p};
		ret.input = ShaderInput(m_vram, spec.meta.spd);
		return ret;
	}
	return std::nullopt;
}

PipelineFactory::Meta PipelineFactory::makeMeta(ShaderSpec const& shader) const {
	ShaderSpec::Map<SpirV> spirV;
	for (auto const& mod : shader.modules) {
		spirV[mod.type] = m_getSpirV(mod.uri);
		EXPECT(!spirV[mod.type].empty());
	}
	Meta ret;
	ret.spd.buffering = m_buffering;
	auto setBindings = utils::extractBindings(std::move(spirV));
	std::vector<vk::DescriptorSetLayout> layouts;
	for (auto& [set, binds] : setBindings.sets) {
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		for (auto& binding : binds) {
			if (binding.binding.descriptorType != vk::DescriptorType()) { bindings.push_back(binding.binding); }
		}
		auto const descLayout = m_vram->m_device->makeDescriptorSetLayout(bindings);
		ret.setLayouts.push_back({m_vram->m_device, descLayout});
		ret.bindings.push_back({bindings.begin(), bindings.end()});
		layouts.push_back(descLayout);

		SetLayoutData sld;
		sld.bindings = ret.bindings.back();
		sld.layout = descLayout;
		ret.spd.sets.push_back(std::move(sld));
	}
	ret.layout = makeDeferred<vk::PipelineLayout>(m_vram->m_device, setBindings.push, layouts);
	return ret;
}

} // namespace le::graphics