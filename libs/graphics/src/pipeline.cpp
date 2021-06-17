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
	if (out_src == T()) { out_src = u; }
}

template <typename T, typename U>
void setIfSet(T& out_dst, U&& u) {
	if (!Device::default_v(u)) { out_dst = u; }
}

bool valid(Shader::ModuleMap const& shaders) noexcept {
	return std::any_of(std::begin(shaders.arr), std::end(shaders.arr), [](vk::ShaderModule const& m) -> bool { return !Device::default_v(m); });
}
} // namespace

Pipeline::Pipeline(not_null<VRAM*> vram, Shader const& shader, CreateInfo info, Hash id) : m_vram(vram), m_device(vram->m_device) {
	m_metadata.main = std::move(info);
	m_storage.id = id;
	vk::Pipeline pipe;
	if (construct(shader, m_metadata.main, pipe, true)) { m_storage.dynamic.main = {m_device, pipe}; }
}

kt::result<vk::Pipeline, void> Pipeline::constructVariant(Hash id, Shader const& shader, CreateInfo::Fixed fixed) {
	if (id == Hash()) { return kt::null_result; }
	vk::Pipeline pipe;
	CreateInfo info = m_metadata.main;
	info.fixedState = fixed;
	if (!construct(shader, info, pipe, false)) { return kt::null_result; }
	m_metadata.variants[id] = std::move(info.fixedState);
	m_storage.dynamic.variants[id] = {m_device, pipe};
	return pipe;
}

kt::result<vk::Pipeline, void> Pipeline::variant(Hash id) const {
	if (id == Hash()) { return *m_storage.dynamic.main; }
	if (auto it = m_storage.dynamic.variants.find(id); it != m_storage.dynamic.variants.end()) { return *it->second; }
	return kt::null_result;
}

bool Pipeline::reconstruct(Shader const& shader) {
	vk::Pipeline pipe;
	auto info = m_metadata.main;
	if (!construct(shader, info, pipe, false)) { return false; }
	m_storage.dynamic.main = {m_device, pipe};
	m_metadata.name = shader.m_name;
	m_metadata.main = std::move(info);
	for (auto& [id, f] : m_metadata.variants) {
		vk::Pipeline pipe;
		auto ci = m_metadata.main;
		ci.fixedState = f;
		if (!construct(shader, ci, pipe, false)) { return false; }
		m_storage.dynamic.variants[id] = {m_device, pipe};
	}
	return true;
}

vk::PipelineBindPoint Pipeline::bindPoint() const { return m_metadata.main.bindPoint; }

vk::PipelineLayout Pipeline::layout() const { return *m_storage.fixed.layout; }

vk::DescriptorSetLayout Pipeline::setLayout(u32 set) const {
	ENSURE(set < (u32)m_storage.fixed.setLayouts.size(), "Set does not exist on pipeline!");
	return *m_storage.fixed.setLayouts[(std::size_t)set];
}

ShaderInput& Pipeline::shaderInput() { return m_storage.input; }

ShaderInput const& Pipeline::shaderInput() const { return m_storage.input; }

SetPool Pipeline::makeSetPool(u32 set, Buffering buffering) const {
	ENSURE(set < (u32)m_storage.fixed.setLayouts.size(), "Set does not exist on pipeline!");
	auto& f = m_storage.fixed;
	if (buffering == 0_B) { buffering = m_metadata.main.buffering; }
	DescriptorSet::CreateInfo const info{m_metadata.name, *f.setLayouts[(std::size_t)set], f.bindingInfos[(std::size_t)set], buffering, set};
	return SetPool(m_device, info);
}

std::unordered_map<u32, SetPool> Pipeline::makeSetPools(Buffering buffering) const {
	std::unordered_map<u32, SetPool> ret;
	auto const& f = m_storage.fixed;
	if (buffering == 0_B) { buffering = m_metadata.main.buffering; }
	for (u32 set = 0; set < (u32)m_storage.fixed.setLayouts.size(); ++set) {
		DescriptorSet::CreateInfo const info{m_metadata.name, *f.setLayouts[(std::size_t)set], f.bindingInfos[(std::size_t)set], buffering, set};
		ret.emplace(set, SetPool(m_device, info));
	}
	return ret;
}

void Pipeline::bindSet(CommandBuffer cb, u32 set, std::size_t idx) const {
	if (m_storage.input.contains(set)) { cb.bindSet(*m_storage.fixed.layout, m_storage.input.set(set).index(idx)); }
}

void Pipeline::bindSet(CommandBuffer cb, std::initializer_list<u32> sets, std::size_t idx) const {
	for (u32 const set : sets) { bindSet(cb, set, idx); }
}

bool Pipeline::construct(Shader const& shader, CreateInfo& out_info, vk::Pipeline& out_pipe, bool bFixed) {
	auto& c = out_info;
	ENSURE(!Device::default_v(c.renderPass), "Invalid render pass");
	ENSURE(valid(shader.m_modules), "Invalid shader m_modules");
	if (!valid(shader.m_modules) || Device::default_v(c.renderPass)) { return false; }
	m_metadata.name = shader.m_name;
	m_storage.cache = out_info.cache;
	if (bFixed) {
		auto& f = m_storage.fixed;
		f = {};
		auto setBindings = utils::extractBindings(shader);
		std::vector<vk::DescriptorSetLayout> layouts;
		for (auto& [set, binds] : setBindings.sets) {
			std::vector<vk::DescriptorSetLayoutBinding> bindings;
			for (auto& setBinding : binds) {
				if (!setBinding.bUnassigned) { bindings.push_back(setBinding.binding); }
			}
			auto const descLayout = m_device->makeDescriptorSetLayout(bindings);
			f.setLayouts.push_back({m_device, descLayout});
			f.bindingInfos.push_back(std::move(binds));
			layouts.push_back(descLayout);
		}
		f.layout = makeDeferred<vk::PipelineLayout>(m_device, setBindings.push, layouts);
		m_storage.input = ShaderInput(*this, m_metadata.main.buffering);
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
		shaderCreateInfo.reserve(arraySize(shader.m_modules.arr));
		for (std::size_t idx = 0; idx < arraySize(shader.m_modules.arr); ++idx) {
			vk::ShaderModule const& module = shader.m_modules.arr[idx];
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
	createInfo.layout = *m_storage.fixed.layout;
	createInfo.renderPass = c.renderPass;
	createInfo.subpass = c.subpass;
	auto result = m_device->device().createGraphicsPipeline(m_storage.cache, createInfo);
	out_pipe = result;
	return true;
}
} // namespace le::graphics
