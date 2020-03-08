#include "engine/vuk/pipeline/pipeline.hpp"
#include "vuk/instance/instance_impl.hpp"

namespace le::vuk
{
Pipeline::Pipeline(Data const& data)
{
	vk::PipelineVertexInputStateCreateInfo vertexInputState;
	{
	}
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
	{
		inputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
		inputAssemblyState.primitiveRestartEnable = false;
	}
	vk::PipelineViewportStateCreateInfo viewportState;
	{
		viewportState.viewportCount = 1;
		viewportState.pViewports = &data.viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &data.scissor;
	}
	vk::PipelineRasterizationStateCreateInfo rasterizerState;
	{
		rasterizerState.depthClampEnable = false;
		rasterizerState.rasterizerDiscardEnable = false;
		rasterizerState.polygonMode = data.polygonMode;
		rasterizerState.lineWidth = data.lineWidth;
		rasterizerState.cullMode = data.cullMode;
		rasterizerState.frontFace = data.frontFace;
		rasterizerState.depthBiasEnable = false;
	}
	vk::PipelineMultisampleStateCreateInfo multisamplerState;
	{
		multisamplerState.sampleShadingEnable = false;
		multisamplerState.rasterizationSamples = vk::SampleCountFlagBits::e1;
	}
	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	{
		colorBlendAttachment.colorWriteMask = data.colourWriteMask;
		colorBlendAttachment.blendEnable = data.bBlend;
	}
	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	{
		colorBlendState.logicOpEnable = false;
		colorBlendState.logicOp = vk::LogicOp::eCopy;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;
	}
	[[maybe_unused]] vk::DynamicState dynamicState;
	{
		// TODO
	}
	auto const& vkDevice = static_cast<vk::Device const&>(*g_pDevice);
	vk::PipelineLayoutCreateInfo layoutCreateInfo;
	layoutCreateInfo.setLayoutCount = 0;
	layoutCreateInfo.pushConstantRangeCount = 0;
	m_layout = vkDevice.createPipelineLayout(layoutCreateInfo);

	vk::GraphicsPipelineCreateInfo createInfo;
	createInfo.stageCount = (u32)data.shaders.size();
	createInfo.pStages = data.shaders.data();
	createInfo.pVertexInputState = &vertexInputState;
	createInfo.pInputAssemblyState = &inputAssemblyState;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterizerState;
	createInfo.pMultisampleState = &multisamplerState;
	createInfo.pDepthStencilState = nullptr;
	createInfo.pColorBlendState = &colorBlendState;
	createInfo.pDynamicState = nullptr;
	createInfo.layout = m_layout;
	createInfo.renderPass = data.renderPass;
	createInfo.subpass = 0;
	vk::PipelineCache pipelineCache;
	m_pipeline = vkDevice.createGraphicsPipeline(pipelineCache, createInfo);
}

Pipeline::~Pipeline()
{
	g_pDevice->destroy(m_pipeline);
	g_pDevice->destroy(m_layout);
}
} // namespace le::vuk
