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
	if (!default_v(u)) {
		out_dst = u;
	}
}

bool valid(Shader::ModuleMap const& shaders) noexcept {
	return std::any_of(shaders.begin(), shaders.end(), [](vk::ShaderModule const& m) -> bool { return !default_v(m); });
}
} // namespace

Pipeline::Pipeline(VRAM& vram, CreateInfo info, Hash id) : m_vram(vram), m_device(vram.m_device) {
	m_metadata.createInfo = std::move(info);
	m_storage.id = id;
	construct(true);
}

Pipeline::Pipeline(Pipeline&& rhs)
	: m_storage(std::move(rhs.m_storage)), m_metadata(std::exchange(rhs.m_metadata, Metadata())), m_vram(rhs.m_vram), m_device(rhs.m_device) {
	rhs.m_storage = {};
}

Pipeline& Pipeline::operator=(Pipeline&& rhs) {
	if (&rhs != this) {
		destroy(true);
		m_storage = std::move(rhs.m_storage);
		m_metadata = std::exchange(rhs.m_metadata, Metadata());
		m_vram = rhs.m_vram;
		m_device = rhs.m_device;
		rhs.m_storage = {};
	}
	return *this;
}

Pipeline::~Pipeline() {
	destroy(true);
}

bool Pipeline::reconstruct(CreateInfo::Dynamic const& newState) {
	destroy(false);
	if (newState.shader) {
		m_metadata.createInfo.dynamicState.shader = newState.shader;
	}
	setIfSet(m_metadata.createInfo.dynamicState.renderPass, newState.renderPass);
	return construct(false);
}

vk::PipelineLayout Pipeline::layout() const {
	return m_storage.fixed.layout;
}

vk::DescriptorSetLayout Pipeline::setLayout(u32 set) const {
	ENSURE(set < (u32)m_storage.fixed.setLayouts.size(), "Set does not exist on pipeline!");
	return m_storage.fixed.setLayouts[(std::size_t)set];
}

SetFactory& Pipeline::setFactory(u32 set) {
	ENSURE(set < (u32)m_storage.setFactories.size(), "Set does not exist on pipeline!");
	return m_storage.setFactories[(std::size_t)set];
}

void Pipeline::swapSets() {
	for (auto& descSet : m_storage.setFactories) {
		descSet.swap();
	}
}

bool Pipeline::writeTextures(SetIndex set, u32 binding, Span<Texture> textures) {
	return descriptorSet(set).writeTextures(binding, textures);
}

void Pipeline::bindSets(CommandBuffer& cmd, Span<DescriptorSet*> descriptorSets) const {
	ENSURE(cmd.rendering(), "Invalid command buffer state");
	u32 firstSet = maths::max<u32>();
	std::vector<vk::DescriptorSet> setsArr;
	setsArr.reserve(descriptorSets.size());
	for (auto const& set : descriptorSets) {
		firstSet = std::min(firstSet, set->setNumber());
		setsArr.push_back(set->get());
	}
	cmd.bindSets(m_storage.fixed.layout, setsArr, firstSet);
}

void Pipeline::bindSet(CommandBuffer& cmd, SetIndex set) {
	bindSets(cmd, &descriptorSet(set));
}

bool Pipeline::construct(bool bFixed) {
	auto& c = m_metadata.createInfo;
	ENSURE(!default_v(c.dynamicState.renderPass), "Invalid render pass");
	ENSURE(c.dynamicState.shader && valid(c.dynamicState.shader->m_modules), "Invalid shader m_modules");
	if (!c.dynamicState.shader || !valid(c.dynamicState.shader->m_modules) || default_v(c.dynamicState.renderPass)) {
		return false;
	}
	Device& d = m_device;
	if (bFixed) {
		m_storage.fixed = {};
		auto const setBindings = utils::extractBindings(c.dynamicState.shader);
		for (auto const& [set, binds] : setBindings.sets) {
			vk::DescriptorSetLayoutCreateInfo createInfo;
			std::vector<vk::DescriptorSetLayoutBinding> bindings;
			for (auto const& setBinding : binds) {
				bindings.push_back(setBinding.binding);
			}
			createInfo.bindingCount = (u32)bindings.size();
			createInfo.pBindings = bindings.data();
			auto const descLayout = d.m_device.createDescriptorSetLayout(createInfo);
			m_storage.fixed.setLayouts.push_back(descLayout);
			SetFactory::CreateInfo const descriptorInfo{descLayout, binds, m_metadata.createInfo.rotateCount, set};
			m_storage.setFactories.emplace_back(m_vram, descriptorInfo);
		}
		m_storage.fixed.layout = d.createPipelineLayout(setBindings.push, m_storage.fixed.setLayouts);
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
	auto& states = m_metadata.createInfo.fixedState.dynamicStates;
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
		shaderCreateInfo.reserve(c.dynamicState.shader->m_modules.size());
		for (std::size_t idx = 0; idx < c.dynamicState.shader->m_modules.size(); ++idx) {
			vk::ShaderModule const& module = c.dynamicState.shader->m_modules[idx];
			if (!default_v(module)) {
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
	createInfo.renderPass = c.dynamicState.renderPass;
	createInfo.subpass = c.subpass;

#if VK_HEADER_VERSION >= 131
	auto pipeline = d.m_device.createGraphicsPipeline({}, createInfo);
#else
	auto [result, pipeline] = d.m_device.createGraphicsPipeline({}, createInfo);
	if (result != vk::Result::eSuccess) {
		return false;
	}
#endif
	m_storage.dynamic.pipeline = pipeline;
	return true;
}

void Pipeline::destroy(bool bFixed) {
	Device& d = m_device;
	if (!default_v(m_storage.dynamic.pipeline)) {
		d.defer([&d, dy = m_storage.dynamic]() mutable { d.destroy(dy.pipeline); });
		m_storage.dynamic = {};
	}
	if (bFixed) {
		d.defer([&d, f = m_storage.fixed]() mutable {
			d.destroy(f.layout);
			for (auto dsl : f.setLayouts) {
				d.destroy(dsl);
			}
		});
		m_storage.fixed = {};
	}
}

DescriptorSet& Pipeline::descriptorSet(SetIndex set) {
	SetFactory& factory = setFactory(set.set);
	factory.populate(set.index + 1);
	return factory.at(set.index);
}
} // namespace le::graphics
