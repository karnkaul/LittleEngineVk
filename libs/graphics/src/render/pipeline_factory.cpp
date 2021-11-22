#include <core/utils/expect.hpp>
#include <graphics/render/context.hpp>
#include <graphics/render/pipeline_factory.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
PipelineData PipelineFactory::Pipe::data() const noexcept { return {&input, pipeline, layout}; }

std::size_t PipelineFactory::Hasher::operator()(Spec const& spec) const {
	utils::HashGen ret;
	return ret << spec.vertexInput << spec.fixedState << spec.shaderURI;
}

PipelineFactory::Spec PipelineFactory::spec(Hash shaderURI, PFlags flags, VertexInputInfo vertexInput) {
	Spec ret;
	EXPECT(shaderURI != Hash());
	ret.shaderURI = shaderURI;
	ret.fixedState.flags = flags;
	ret.vertexInput = vertexInput.bindings.empty() ? VertexInfoFactory<Vertex>{}(0) : std::move(vertexInput);
	return ret;
}

PipelineFactory::PipelineFactory(not_null<VRAM*> vram, GetShader&& getShader, Buffering buffering) noexcept
	: m_getShader(std::move(getShader)), m_vram(vram), m_buffering(buffering) {
	EXPECT(m_getShader.has_value());
}

PipelineData PipelineFactory::get(Spec const& spec, vk::RenderPass renderPass) {
	EXPECT(spec.shaderURI != Hash());
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
	auto pit = specMap.map.find(renderPass);
	if (pit == specMap.map.end() || pit->second.refresh) {
		auto pipe = make(spec, renderPass);
		ENSURE(pipe, "Failed to create pipeline");
		auto [i, b] = specMap.map.insert_or_assign(renderPass, std::move(*pipe));
		pit = i;
	}
	return pit->second.data();
}

bool PipelineFactory::contains(Hash spec, vk::RenderPass renderPass) const {
	if (auto sit = m_storage.find(spec); sit != m_storage.end()) { return sit->second.map.contains(renderPass); }
	return false;
}

PipelineSpec const* PipelineFactory::find(Hash spec) const {
	if (auto sit = m_storage.find(spec); sit != m_storage.end()) { return &sit->second.spec; }
	return nullptr;
}

std::size_t PipelineFactory::setDirty(Hash spec) {
	if (auto sit = m_storage.find(spec); sit != m_storage.end()) {
		for (auto& [_, pipe] : sit->second.map) { pipe.refresh = true; }
		return sit->second.map.size();
	}
	return {};
}

std::size_t PipelineFactory::pipeCount(Hash spec) const noexcept {
	if (auto sit = m_storage.find(spec); sit != m_storage.end()) { return sit->second.map.size(); }
	return {};
}

void PipelineFactory::clear(Hash spec) noexcept {
	if (auto sit = m_storage.find(spec); sit != m_storage.end()) { sit->second.map.clear(); }
}

std::optional<PipelineFactory::Pipe> PipelineFactory::make(Spec const& spec, vk::RenderPass renderPass) const {
	auto const& shader = m_getShader(spec.shaderURI);
	EXPECT(utils::hasActiveModule(shader.m_modules));
	if (!utils::hasActiveModule(shader.m_modules)) { return std::nullopt; }
	Pipe ret;
	auto spd = setLayout(ret, shader);
	utils::PipeData data;
	data.renderPass = renderPass;
	data.layout = ret.layout;
	// TODO
	// in.cache = m_pipelineCache;
	if (auto p = utils::makeGraphicsPipeline(*m_vram->m_device, shader.m_modules, spec, data)) {
		ret.pipeline = Deferred<vk::Pipeline>{m_vram->m_device, *p};
		ret.input = ShaderInput(m_vram, std::move(spd));
		return ret;
	}
	return std::nullopt;
}

SetPoolsData PipelineFactory::setLayout(Pipe& out_pipe, Shader const& shader) const {
	Shader::CodeMap spirV;
	auto setBindings = utils::extractBindings(shader.m_spirV);
	SetPoolsData ret;
	std::vector<vk::DescriptorSetLayout> layouts;
	for (auto& [set, binds] : setBindings.sets) {
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		for (auto& setBinding : binds) {
			if (!setBinding.bUnassigned) { bindings.push_back(setBinding.binding); }
		}
		auto const descLayout = m_vram->m_device->makeDescriptorSetLayout(bindings);
		out_pipe.setLayouts.push_back({m_vram->m_device, descLayout});
		out_pipe.bindingInfos.push_back(std::move(binds));
		layouts.push_back(descLayout);

		SetLayoutData sld;
		sld.bindings = out_pipe.bindingInfos.back();
		sld.layout = descLayout;
		ret.sets.push_back(std::move(sld));
	}
	out_pipe.layout = makeDeferred<vk::PipelineLayout>(m_vram->m_device, setBindings.push, layouts);
	return ret;
}
} // namespace le::graphics
