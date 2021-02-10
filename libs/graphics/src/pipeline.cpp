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
	m_metadata.createInfo = std::move(info);
	m_storage.id = id;
	construct(shader, m_storage.dynamic.pipeline, true);
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

bool Pipeline::reconstruct(Shader const& shader) {
	vk::Pipeline pipe;
	if (construct(shader, pipe, false)) {
		destroy(false);
		m_storage.dynamic.pipeline = pipe;
		return true;
	}
	return false;
}

vk::PipelineLayout Pipeline::layout() const {
	return m_storage.fixed.layout;
}

vk::DescriptorSetLayout Pipeline::setLayout(u32 set) const {
	ENSURE(set < (u32)m_storage.fixed.setLayouts.size(), "Set does not exist on pipeline!");
	return m_storage.fixed.setLayouts[(std::size_t)set];
}

SetPool Pipeline::makeSetPool(u32 set, std::size_t rotateCount) const {
	ENSURE(set < (u32)m_storage.fixed.setLayouts.size(), "Set does not exist on pipeline!");
	auto& f = m_storage.fixed;
	if (rotateCount == 0) {
		rotateCount = m_metadata.createInfo.rotateCount;
	}
	DescriptorSet::CreateInfo const info{f.setLayouts[(std::size_t)set], f.bindingInfos[(std::size_t)set], rotateCount, set};
	return SetPool(m_device, info);
}

std::unordered_map<u32, SetPool> Pipeline::makeSetPools(std::size_t rotateCount) const {
	std::unordered_map<u32, SetPool> ret;
	auto const& f = m_storage.fixed;
	if (rotateCount == 0) {
		rotateCount = m_metadata.createInfo.rotateCount;
	}
	for (u32 set = 0; set < (u32)m_storage.fixed.setLayouts.size(); ++set) {
		DescriptorSet::CreateInfo const info{f.setLayouts[(std::size_t)set], f.bindingInfos[(std::size_t)set], rotateCount, set};
		ret.emplace(set, SetPool(m_device, info));
	}
	return ret;
}

bool Pipeline::construct(Shader const& shader, vk::Pipeline& out_pipe, bool bFixed) {
	auto& c = m_metadata.createInfo;
	ENSURE(!Device::default_v(c.renderPass), "Invalid render pass");
	ENSURE(valid(shader.m_modules), "Invalid shader m_modules");
	if (!valid(shader.m_modules) || Device::default_v(c.renderPass)) {
		return false;
	}
	if (bFixed) {
		auto& f = m_storage.fixed;
		f = {};
		auto setBindings = utils::extractBindings(shader);
		for (auto& [set, binds] : setBindings.sets) {
			vk::DescriptorSetLayoutCreateInfo createInfo;
			std::vector<vk::DescriptorSetLayoutBinding> bindings;
			for (auto& setBinding : binds) {
				bindings.push_back(setBinding.binding);
			}
			createInfo.bindingCount = (u32)bindings.size();
			createInfo.pBindings = bindings.data();
			auto const descLayout = m_device.get().device().createDescriptorSetLayout(createInfo);
			f.setLayouts.push_back(descLayout);
			f.bindingInfos.push_back(std::move(binds));
		}
		f.layout = m_device.get().createPipelineLayout(setBindings.push, f.setLayouts);
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
	auto result = m_device.get().device().createGraphicsPipeline({}, createInfo);
	out_pipe = result;
	return true;
}

void Pipeline::destroy(bool bFixed) {
	Device& d = m_device;
	if (!Device::default_v(m_storage.dynamic.pipeline)) {
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
} // namespace le::graphics
