#include <fmt/format.h>
#include "core/log.hpp"
#include "engine/assets/resources.hpp"
#include "engine/gfx/pipeline.hpp"
#include "engine/gfx/shader.hpp"
#include "deferred.hpp"
#include "device.hpp"
#include "presenter.hpp"
#include "pipeline_impl.hpp"
#include "resource_descriptors.hpp"

namespace le::gfx
{
std::string const PipelineImpl::s_tName = utils::tName<Pipeline>();

Pipeline::Pipeline() = default;
Pipeline::Pipeline(Pipeline&&) = default;
Pipeline& Pipeline::operator=(Pipeline&&) = default;
Pipeline::~Pipeline() = default;

PipelineImpl::PipelineImpl(Pipeline* pPipeline) : m_pPipeline(pPipeline) {}

PipelineImpl::PipelineImpl(PipelineImpl&&) = default;
PipelineImpl& PipelineImpl::operator=(PipelineImpl&&) = default;

PipelineImpl::~PipelineImpl()
{
	destroy();
}

bool PipelineImpl::create(Info info)
{
	m_info = std::move(info);
	ASSERT(m_info.samplerLayout != vk::DescriptorSetLayout(), "Null descriptor set layout!");
	if (m_info.shaderID.empty())
	{
		m_info.shaderID = "shaders/default";
	}
	m_pPipeline->m_name = fmt::format("{}:{}-?", m_info.window, m_info.name);
	if (create())
	{
		m_pPipeline->m_name = fmt::format("{}:{}-{}", m_info.window, m_info.name, m_info.pShader->m_id.generic_string());
		LOG_D("[{}] [{}] created", s_tName, m_pPipeline->m_name);
		return true;
	}
	return false;
}

bool PipelineImpl::update(vk::DescriptorSetLayout samplerLayout)
{
	bool bOutOfDate = samplerLayout != vk::DescriptorSetLayout() && samplerLayout != m_info.samplerLayout;
#if defined(LEVK_ASSET_HOT_RELOAD)
	bOutOfDate |= m_bShaderReloaded;
	m_bShaderReloaded = false;
#endif
	if (bOutOfDate)
	{
		// Add a frame of padding since this frame hasn't completed drawing yet
		deferred::release([pipeline = m_pipeline, layout = m_layout]() { g_device.destroy(pipeline, layout); }, 1);
		m_info.samplerLayout = samplerLayout;
		if (create())
		{
			LOG_D("[{}] [{}] recreated", s_tName, m_pPipeline->m_name);
			return true;
		}
		return false;
	}
	return true;
}

void PipelineImpl::destroy()
{
	deferred::release(m_pipeline, m_layout);
	LOGIF_D(m_pipeline != vk::Pipeline(), "[{}] [{}] destroyed", s_tName, m_pPipeline->m_name);
	m_pipeline = vk::Pipeline();
	m_layout = vk::PipelineLayout();
	return;
}

#if defined(LEVK_ASSET_HOT_RELOAD)
void PipelineImpl::pollShaders()
{
	m_bShaderReloaded |= m_info.pShader && m_info.pShader->currentStatus() == Asset::Status::eReloaded;
}
#endif

bool PipelineImpl::create()
{
	if (!m_info.pShader && !m_info.shaderID.empty())
	{
		m_info.pShader = Resources::inst().get<Shader>(m_info.shaderID);
	}
	ASSERT(m_info.pShader, "Shader is null!");
	if (!m_info.pShader)
	{
		LOG_E("[{}] [{}] Failed to create pipeline!", s_tName, m_pPipeline->m_name);
		return false;
	}
	auto const bindingDescription = vbo::bindingDescription();
	auto const attributeDescriptions = vbo::attributeDescriptions();
	std::vector const setLayouts = {rd::g_bufferLayout, m_info.samplerLayout};
	m_layout = createPipelineLayout(m_info.pushConstantRanges, setLayouts);
	vk::PipelineVertexInputStateCreateInfo vertexInputState;
	{
		vertexInputState.vertexAttributeDescriptionCount = (u32)attributeDescriptions.size();
		vertexInputState.vertexBindingDescriptionCount = 1;
		vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputState.pVertexBindingDescriptions = &bindingDescription;
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
	vk::PipelineRasterizationStateCreateInfo rasterizerState;
	{
		rasterizerState.depthClampEnable = false;
		rasterizerState.rasterizerDiscardEnable = false;
		rasterizerState.polygonMode = m_info.polygonMode;
		rasterizerState.lineWidth = m_info.staticLineWidth;
		rasterizerState.cullMode = m_info.cullMode;
		rasterizerState.frontFace = m_info.frontFace;
		rasterizerState.depthBiasEnable = false;
	}
	vk::PipelineMultisampleStateCreateInfo multisamplerState;
	{
		multisamplerState.sampleShadingEnable = false;
		multisamplerState.rasterizationSamples = vk::SampleCountFlagBits::e1;
	}
	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	{
		colorBlendAttachment.colorWriteMask = m_info.colourWriteMask;
		colorBlendAttachment.blendEnable = m_info.bBlend;
		colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
		colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
	}
	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	{
		colorBlendState.logicOpEnable = false;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;
	}
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	{
		depthStencilState.depthTestEnable = m_info.bDepthTest;
		depthStencilState.depthWriteEnable = m_info.bDepthWrite;
		depthStencilState.depthCompareOp = vk::CompareOp::eLess;
	}
	auto states = m_info.dynamicStates;
	states.insert(vk::DynamicState::eViewport);
	states.insert(vk::DynamicState::eScissor);
	std::vector<vk::DynamicState> stateFlags = {states.begin(), states.end()};
	vk::PipelineDynamicStateCreateInfo dynamicState;
	{
		dynamicState.dynamicStateCount = (u32)stateFlags.size();
		dynamicState.pDynamicStates = stateFlags.data();
	}
	std::vector<vk::PipelineShaderStageCreateInfo> shaderCreateInfo;
	{
		auto modules = m_info.pShader->m_uImpl->modules();
		shaderCreateInfo.reserve(modules.size());
		for (auto const& [type, module] : modules)
		{
			vk::PipelineShaderStageCreateInfo createInfo;
			createInfo.stage = ShaderImpl::s_typeToFlagBit[(size_t)type];
			createInfo.module = module;
			createInfo.pName = "main";
			shaderCreateInfo.push_back(std::move(createInfo));
		}
	}
	vk::GraphicsPipelineCreateInfo createInfo;
	createInfo.stageCount = (u32)shaderCreateInfo.size();
	createInfo.pStages = shaderCreateInfo.data();
	createInfo.pVertexInputState = &vertexInputState;
	createInfo.pInputAssemblyState = &inputAssemblyState;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterizerState;
	createInfo.pMultisampleState = &multisamplerState;
	createInfo.pDepthStencilState = &depthStencilState;
	createInfo.pColorBlendState = &colorBlendState;
	createInfo.pDynamicState = &dynamicState;
	createInfo.layout = m_layout;
	createInfo.renderPass = m_info.renderPass;
	createInfo.subpass = 0;
	m_pipeline = g_device.device.createGraphicsPipeline({}, createInfo);
#if defined(LEVK_ASSET_HOT_RELOAD)
	m_bShaderReloaded = false;
#endif
	return true;
}
} // namespace le::gfx
