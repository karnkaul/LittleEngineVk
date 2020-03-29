#include <fmt/format.h>
#include "core/log.hpp"
#include "window/window_impl.hpp"
#include "gfx/info.hpp"
#include "gfx/presenter.hpp"
#include "gfx/utils.hpp"
#include "pipeline.hpp"
#include "resources.hpp"
#include "shader.hpp"
#include "vertex.hpp"

namespace le::gfx
{
std::string const Pipeline::s_tName = utils::tName<Pipeline>();

Pipeline::Pipeline(WindowID presenterWindow) : m_window(presenterWindow) {}

Pipeline::~Pipeline()
{
	destroy();
}

bool Pipeline::create(Info info)
{
	m_info = std::move(info);
	ASSERT(m_info.pShader, "Shader is null!");
	if (!m_info.pShader)
	{
		LOG_E("[{}] [{}] Failed to create pipeline!", s_tName, m_info.name);
		return false;
	}
	m_name = fmt::format("{}-{}", m_info.name, m_info.pShader->m_id);
	return recreate();
}

bool Pipeline::recreate()
{
	destroy();
	auto const pPresenter = WindowImpl::presenter(m_window);
	ASSERT(pPresenter, "Presneter is null!");
	if (!m_info.pShader && g_pResources && !m_info.shaderID.empty())
	{
		m_info.pShader = g_pResources->get<Shader>(m_info.shaderID);
	}
	ASSERT(m_info.pShader, "Shader is null!");
	if (!m_info.pShader || !pPresenter)
	{
		LOG_E("[{}] [{}] Failed to create pipeline!", s_tName, m_name);
		return false;
	}
	auto const bindingDescription = Vertex::bindingDescription();
	auto const attributeDescriptions = Vertex::attributeDescriptions();
	{
		vk::PipelineLayoutCreateInfo layoutCreateInfo;
		layoutCreateInfo.setLayoutCount = (u32)m_info.setLayouts.size();
		layoutCreateInfo.pSetLayouts = m_info.setLayouts.data();
		vk::PushConstantRange pushConstantRange;
		pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::mat4);
		layoutCreateInfo.pushConstantRangeCount = 1;
		layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		m_layout = gfx::g_info.device.createPipelineLayout(layoutCreateInfo);
	}
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
	}
	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	{
		colorBlendState.logicOpEnable = false;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;
	}
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	{
		depthStencilState.depthTestEnable = true;
		depthStencilState.depthWriteEnable = true;
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
		auto modules = m_info.pShader->modules();
		shaderCreateInfo.reserve(modules.size());
		for (auto const& [type, module] : modules)
		{
			vk::PipelineShaderStageCreateInfo createInfo;
			createInfo.stage = Shader::s_typeToFlagBit[(size_t)type];
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
	createInfo.renderPass = pPresenter->m_renderPass;
	createInfo.subpass = 0;
	m_pipeline = g_info.device.createGraphicsPipeline({}, createInfo);
	LOG_D("[{}] [{}] created", s_tName, m_name);
	return true;
}

void Pipeline::destroy()
{
	vkDestroy(m_pipeline, m_layout);
	LOGIF_D(m_pipeline != vk::Pipeline(), "[{}] [{}] destroyed", s_tName, m_name);
	m_pipeline = vk::Pipeline();
	m_layout = vk::PipelineLayout();
}

void Pipeline::update()
{
#if defined(LEVK_ASSET_HOT_RELOAD)
	if (m_info.pShader && m_info.pShader->currentStatus() == Resource::Status::eReloaded)
	{
		g_info.device.waitIdle();
		recreate();
	}
#endif
}

vk::Pipeline Pipeline::pipeline() const
{
	return m_pipeline;
}

vk::PipelineLayout Pipeline::layout() const
{
	return m_layout;
}
} // namespace le::gfx
