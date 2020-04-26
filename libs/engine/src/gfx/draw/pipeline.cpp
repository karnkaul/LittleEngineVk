#include <fmt/format.h>
#include "core/log.hpp"
#include "engine/assets/resources.hpp"
#include "engine/gfx/shader.hpp"
#include "gfx/info.hpp"
#include "gfx/presenter.hpp"
#include "gfx/utils.hpp"
#include "pipeline.hpp"

namespace le::gfx
{
std::string const Pipeline::s_tName = utils::tName<Pipeline>();

Pipeline::Pipeline() = default;

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
	m_name = fmt::format("{}-{}", m_info.name, m_info.pShader->m_id.generic_string());
	return create(m_pipeline, m_layout);
}

void Pipeline::destroy()
{
	waitAll(m_activeFences);
	destroy(m_pipeline, m_layout);
	m_activeFences.clear();
#if defined(LEVK_ASSET_HOT_RELOAD)
	waitAll(m_standby.drawing);
	m_standby.drawing.clear();
	if (m_standby.bReady)
	{
		destroy(m_standby.pipeline, m_standby.layout);
	}
#endif
}

bool Pipeline::create(vk::Pipeline& out_pipeline, vk::PipelineLayout& out_layout)
{
	if (!m_info.pShader && g_pResources && !m_info.shaderID.empty())
	{
		m_info.pShader = g_pResources->get<Shader>(m_info.shaderID);
	}
	ASSERT(m_info.pShader, "Shader is null!");
	if (!m_info.pShader)
	{
		LOG_E("[{}] [{}] Failed to create pipeline!", s_tName, m_name);
		return false;
	}
	auto const bindingDescription = vbo::bindingDescription();
	auto const attributeDescriptions = vbo::attributeDescriptions();
	{
		vk::PipelineLayoutCreateInfo layoutCreateInfo;
		layoutCreateInfo.setLayoutCount = (u32)m_info.setLayouts.size();
		layoutCreateInfo.pSetLayouts = m_info.setLayouts.data();
		layoutCreateInfo.pushConstantRangeCount = (u32)m_info.pushConstantRanges.size();
		layoutCreateInfo.pPushConstantRanges = m_info.pushConstantRanges.data();
		out_layout = gfx::g_info.device.createPipelineLayout(layoutCreateInfo);
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
	out_pipeline = g_info.device.createGraphicsPipeline({}, createInfo);
	LOG_D("[{}] [{}] created", s_tName, m_name);
	return true;
}

void Pipeline::destroy(vk::Pipeline& out_pipeline, vk::PipelineLayout& out_layout)
{
	vkDestroy(out_pipeline, out_layout);
	LOGIF_D(out_pipeline != vk::Pipeline(), "[{}] [{}] destroyed", s_tName, m_name);
	out_pipeline = vk::Pipeline();
	out_layout = vk::PipelineLayout();
}

void Pipeline::update()
{
	auto fenceSignalled = [](auto fence) { return isSignalled(fence); };
	m_activeFences.erase(std::remove_if(m_activeFences.begin(), m_activeFences.end(), fenceSignalled), m_activeFences.end());
#if defined(LEVK_ASSET_HOT_RELOAD)
	if (m_standby.bReady)
	{
		auto& dr = m_standby.drawing;
		dr.erase(std::remove_if(dr.begin(), dr.end(), fenceSignalled), dr.end());
		if (dr.empty())
		{
			destroy(m_pipeline, m_layout);
			m_pipeline = m_standby.pipeline;
			m_layout = m_standby.layout;
			m_standby = {};
			LOG_D("[{}] [{}] ...recreated", s_tName, m_name);
		}
	}
	if (m_info.pShader && m_info.pShader->currentStatus() == Asset::Status::eReloaded)
	{
		LOG_D("[{}] [{}] recreating...", s_tName, m_name);
		m_standby.bReady = create(m_standby.pipeline, m_standby.layout);
		m_standby.drawing = std::move(m_activeFences);
		m_activeFences.clear();
	}
#endif
}

void Pipeline::attach(vk::Fence drawing)
{
	ASSERT(drawing != vk::Fence(), "Invalid fence!");
	m_activeFences.push_back(drawing);
}
} // namespace le::gfx
