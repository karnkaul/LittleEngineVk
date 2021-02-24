#include <algorithm>
#include <core/maths.hpp>
#include <graphics/context/command_buffer.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/shader.hpp>
#include <graphics/utils/utils.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
namespace {
template <typename T, typename U>
void ensureSet(T& out_src, U&& u) {
	if (out_src == T()) {
		out_src = u;
	}
}

template <typename T, typename U>
void setIfSet(T& out_dst, U&& u) {
	if (!Device::default_v(u)) {
		out_dst = u;
	}
}

bool valid(Shader::ModuleMap const& shaders) noexcept {
	return std::any_of(shaders.begin(), shaders.end(), [](vk::ShaderModule const& m) -> bool { return !Device::default_v(m); });
}
} // namespace

Pipeline::Pipeline(VRAM& vram, Shader const& shader, CreateInfo info, Hash id) : m_vram(vram), m_device(vram.m_device) {
	m_metadata.main = std::move(info);
	m_storage.id = id;
	construct(shader, m_metadata.main, m_storage.dynamic.main, true);
}

Pipeline::Pipeline(Pipeline&& rhs) : m_storage(std::move(rhs.m_storage)), m_metadata(std::move(rhs.m_metadata)), m_vram(rhs.m_vram), m_device(rhs.m_device) {
	rhs.m_storage = {};
	rhs.m_metadata = {};
}

Pipeline& Pipeline::operator=(Pipeline&& rhs) {
	if (&rhs != this) {
		destroy();
		m_storage = std::move(rhs.m_storage);
		m_metadata = std::move(rhs.m_metadata);
		m_vram = rhs.m_vram;
		m_device = rhs.m_device;
		rhs.m_storage = {};
		rhs.m_metadata = {};
	}
	return *this;
}

Pipeline::~Pipeline() {
	destroy();
}

kt::result_t<vk::Pipeline, void> Pipeline::constructVariant(Hash id, Shader const& shader, CreateInfo::Fixed fixed) {
	if (id == Hash()) {
		return kt::null_result;
	}
	vk::Pipeline pipe;
	CreateInfo info = m_metadata.main;
	info.fixedState = fixed;
	if (!construct(shader, info, pipe, false)) {
		return kt::null_result;
	}
	m_metadata.variants[id] = std::move(info.fixedState);
	m_storage.dynamic.variants[id] = pipe;
	return pipe;
}

kt::result_t<vk::Pipeline, void> Pipeline::variant(Hash id) const {
	if (id == Hash()) {
		return m_storage.dynamic.main;
	}
	if (auto it = m_storage.dynamic.variants.find(id); it != m_storage.dynamic.variants.end()) {
		return it->second;
	}
	return kt::null_result;
}

bool Pipeline::reconstruct(Shader const& shader) {
	vk::Pipeline pipe;
	auto info = m_metadata.main;
	if (!construct(shader, info, pipe, false)) {
		return false;
	}
	destroy(m_storage.dynamic.main);
	m_storage.dynamic.main = pipe;
	m_metadata.main = std::move(info);
	for (auto& [id, f] : m_metadata.variants) {
		vk::Pipeline pipe;
		auto ci = m_metadata.main;
		ci.fixedState = f;
		if (!construct(shader, ci, pipe, false)) {
			return false;
		}
		destroy(m_storage.dynamic.variants[id]);
		m_storage.dynamic.variants[id] = pipe;
	}
	return true;
}

vk::PipelineBindPoint Pipeline::bindPoint() const {
	return m_metadata.main.bindPoint;
}

vk::PipelineLayout Pipeline::layout() const {
	return m_storage.fixed.layout;
}

vk::DescriptorSetLayout Pipeline::setLayout(u32 set) const {
	ENSURE(set < (u32)m_storage.fixed.setLayouts.size(), "Set does not exist on pipeline!");
	return m_storage.fixed.setLayouts[(std::size_t)set];
}

ShaderInput& Pipeline::shaderInput() {
	return m_storage.input;
}

ShaderInput const& Pipeline::shaderInput() const {
	return m_storage.input;
}

SetPool Pipeline::makeSetPool(u32 set, std::size_t rotateCount) const {
	ENSURE(set < (u32)m_storage.fixed.setLayouts.size(), "Set does not exist on pipeline!");
	auto& f = m_storage.fixed;
	if (rotateCount == 0) {
		rotateCount = m_metadata.main.rotateCount;
	}
	DescriptorSet::CreateInfo const info{f.setLayouts[(std::size_t)set], f.bindingInfos[(std::size_t)set], rotateCount, set};
	return SetPool(m_device, info);
}

std::unordered_map<u32, SetPool> Pipeline::makeSetPools(std::size_t rotateCount) const {
	std::unordered_map<u32, SetPool> ret;
	auto const& f = m_storage.fixed;
	if (rotateCount == 0) {
		rotateCount = m_metadata.main.rotateCount;
	}
	for (u32 set = 0; set < (u32)m_storage.fixed.setLayouts.size(); ++set) {
		DescriptorSet::CreateInfo const info{f.setLayouts[(std::size_t)set], f.bindingInfos[(std::size_t)set], rotateCount, set};
		ret.emplace(set, SetPool(m_device, info));
	}
	return ret;
}

bool Pipeline::construct(Shader const& shader, CreateInfo& out_info, vk::Pipeline& out_pipe, bool bFixed) {
	auto& c = out_info;
	ENSURE(!Device::default_v(c.renderPass), "Invalid render pass");
	ENSURE(valid(shader.m_modules), "Invalid shader m_modules");
	if (!valid(shader.m_modules) || Device::default_v(c.renderPass)) {
		return false;
	}
	if (Device::default_v(m_storage.dynamic.cache)) {
		m_storage.dynamic.cache = m_device.get().device().createPipelineCache({});
	}
	if (bFixed) {
		auto& f = m_storage.fixed;
		f = {};
		auto setBindings = utils::extractBindings(shader);
		for (auto& [set, binds] : setBindings.sets) {
			vk::DescriptorSetLayoutCreateInfo createInfo;
			std::vector<vk::DescriptorSetLayoutBinding> bindings;
			for (auto& setBinding : binds) {
				if (!setBinding.bUnassigned) {
					bindings.push_back(setBinding.binding);
				}
			}
			createInfo.bindingCount = (u32)bindings.size();
			createInfo.pBindings = bindings.data();
			auto const descLayout = m_device.get().device().createDescriptorSetLayout(createInfo);
			f.setLayouts.push_back(descLayout);
			f.bindingInfos.push_back(std::move(binds));
		}
		f.layout = m_device.get().createPipelineLayout(setBindings.push, f.setLayouts);
		m_storage.input = ShaderInput(*this, m_metadata.main.rotateCount);
	}
	vk::PipelineVertexInputStateCreateInfo vertexInputState;
	{
		auto const& vi = c.fixedState.vertexInput;
		vertexInputState.pVertexBindingDescriptions = vi.bindings.data();
		vertexInputState.vertexBindingDescriptionCount = (u32)vi.bindings.size();
		vertexInputState.pVertexAttributeDescriptions = vi.attributes.data();
		vertexInputState.vertexAttributeDescriptionCount = (u32)vi.attributes.size();
	}
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
	{
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		inputAssemblyState.primitiveRestartEnable = false;
	}
	vk::PipelineViewportStateCreateInfo viewportState;
	{
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
	}
	{
		c.fixedState.rasterizerState.depthClampEnable = false;
		c.fixedState.rasterizerState.rasterizerDiscardEnable = false;
		c.fixedState.rasterizerState.depthBiasEnable = false;
		ensureSet(c.fixedState.rasterizerState.lineWidth, 1.0f);
	}
	{
		using CC = vk::ColorComponentFlagBits;
		ensureSet(c.fixedState.colorBlendAttachment.colorWriteMask, CC::eR | CC::eG | CC::eB | CC::eA);
		ensureSet(c.fixedState.colorBlendAttachment.srcColorBlendFactor, vk::BlendFactor::eSrcAlpha);
		ensureSet(c.fixedState.colorBlendAttachment.dstColorBlendFactor, vk::BlendFactor::eOneMinusSrcAlpha);
		ensureSet(c.fixedState.colorBlendAttachment.srcAlphaBlendFactor, vk::BlendFactor::eOne);
	}
	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	{
		colorBlendState.logicOpEnable = false;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &c.fixedState.colorBlendAttachment;
	}
	{ ensureSet(c.fixedState.depthStencilState.depthCompareOp, vk::CompareOp::eLess); }
	auto& states = c.fixedState.dynamicStates;
	states.insert(vk::DynamicState::eViewport);
	states.insert(vk::DynamicState::eScissor);
	std::vector<vk::DynamicState> const stateFlags = {states.begin(), states.end()};
	vk::PipelineDynamicStateCreateInfo dynamicState;
	{
		dynamicState.dynamicStateCount = (u32)stateFlags.size();
		dynamicState.pDynamicStates = stateFlags.data();
	}
	std::vector<vk::PipelineShaderStageCreateInfo> shaderCreateInfo;
	{
		shaderCreateInfo.reserve(shader.m_modules.size());
		for (std::size_t idx = 0; idx < shader.m_modules.size(); ++idx) {
			vk::ShaderModule const& module = shader.m_modules[idx];
			if (!Device::default_v(module)) {
				vk::PipelineShaderStageCreateInfo createInfo;
				createInfo.stage = Shader::typeToFlag[idx];
				createInfo.module = module;
				createInfo.pName = "main";
				shaderCreateInfo.push_back(std::move(createInfo));
			}
		}
	}

	vk::GraphicsPipelineCreateInfo createInfo;
	createInfo.stageCount = (u32)shaderCreateInfo.size();
	createInfo.pStages = shaderCreateInfo.data();
	createInfo.pVertexInputState = &vertexInputState;
	createInfo.pInputAssemblyState = &inputAssemblyState;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &c.fixedState.rasterizerState;
	createInfo.pMultisampleState = &c.fixedState.multisamplerState;
	createInfo.pDepthStencilState = &c.fixedState.depthStencilState;
	createInfo.pColorBlendState = &colorBlendState;
	createInfo.pDynamicState = &dynamicState;
	createInfo.layout = m_storage.fixed.layout;
	createInfo.renderPass = c.renderPass;
	createInfo.subpass = c.subpass;
	auto result = m_device.get().device().createGraphicsPipeline(m_storage.dynamic.cache, createInfo);
	out_pipe = result;
	return true;
}

void Pipeline::destroy() {
	Device& d = m_device;
	destroy(m_storage.dynamic.main);
	for (auto const& [_, pipe] : m_storage.dynamic.variants) {
		destroy(pipe);
	}
	d.defer([&d, f = m_storage.fixed, c = m_storage.dynamic.cache]() mutable {
		d.destroy(f.layout);
		d.destroy(c);
		for (auto dsl : f.setLayouts) {
			d.destroy(dsl);
		}
	});
	m_storage = {};
}

void Pipeline::destroy(vk::Pipeline pipeline) {
	if (!Device::default_v(pipeline)) {
		Device& d = m_device;
		d.defer([&d, pipeline]() mutable { d.destroy(pipeline); });
	}
}
} // namespace le::graphics
