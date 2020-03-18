#include "info.hpp"
#include "context.hpp"
#include "utils.hpp"
#include "shader.hpp"
#include "draw/vertex.hpp"

namespace le
{
vuk::VkResource<vk::Image> vuk::createImage(ImageData const& data)
{
	vk::ImageCreateInfo imageInfo = {};
	imageInfo.imageType = data.type;
	imageInfo.extent = data.size;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = data.format;
	imageInfo.tiling = data.tiling;
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = data.usage;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	auto const queueIndices = g_info.uniqueQueueIndices(false, true);
	imageInfo.queueFamilyIndexCount = (u32)queueIndices.size();
	imageInfo.pQueueFamilyIndices = queueIndices.data();
	imageInfo.sharingMode = g_info.sharingMode(false, true);
	auto image = g_info.device.createImage(imageInfo);

	vk::MemoryRequirements memRequirements = g_info.device.getImageMemoryRequirements(image);
	vk::MemoryAllocateInfo allocInfo = {};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = g_info.findMemoryType(memRequirements.memoryTypeBits, data.properties);
	auto memory = g_info.device.allocateMemory(allocInfo);
	g_info.device.bindImageMemory(image, memory, 0);
	return {image, memory};
}

vk::ImageView vuk::createImageView(vk::Image image, vk::ImageViewType type, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
	vk::ImageViewCreateInfo createInfo;
	createInfo.image = image;
	createInfo.viewType = type;
	createInfo.format = format;
	createInfo.components.r = createInfo.components.g = createInfo.components.b = createInfo.components.a = vk::ComponentSwizzle::eIdentity;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;
	return g_info.device.createImageView(createInfo);
}

vuk::VkResource<vk::Buffer> vuk::createBuffer(BufferData const& data)
{
	vk::Buffer buffer;
	vk::DeviceMemory bufferMemory;
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = data.size;
	bufferInfo.usage = data.usage;
	bufferInfo.sharingMode = vuk::g_info.sharingMode(false, true);
	auto const queues = vuk::g_info.uniqueQueueIndices(false, true);
	bufferInfo.queueFamilyIndexCount = (u32)queues.size();
	bufferInfo.pQueueFamilyIndices = queues.data();
	buffer = vuk::g_info.device.createBuffer(bufferInfo);
	vk::MemoryRequirements memRequirements = vuk::g_info.device.getBufferMemoryRequirements(buffer);
	vk::MemoryAllocateInfo allocInfo = {};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = vuk::g_info.findMemoryType(memRequirements.memoryTypeBits, data.properties);
	bufferMemory = vuk::g_info.device.allocateMemory(allocInfo);
	vuk::g_info.device.bindBufferMemory(buffer, bufferMemory, 0);
	return {buffer, bufferMemory, data.size};
}

void vuk::copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, TransferOp* pOp)
{
	ASSERT(pOp, "Null pointer!");
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = pOp->pool;
	allocInfo.commandBufferCount = 1;
	auto commandBuffer = g_info.device.allocateCommandBuffers(allocInfo).front();
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer.begin(beginInfo);
	vk::BufferCopy copyRegion;
	copyRegion.size = size;
	commandBuffer.copyBuffer(src, dst, copyRegion);
	commandBuffer.end();
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	pOp->transferred = g_info.device.createFence({});
	pOp->queue.submit(submitInfo, pOp->transferred);
	return;
}

vk::RenderPass vuk::createRenderPass(vk::Format format)
{
	vk::AttachmentDescription colourDesc;
	colourDesc.format = format;
	colourDesc.samples = vk::SampleCountFlagBits::e1;
	colourDesc.loadOp = vk::AttachmentLoadOp::eClear;
	colourDesc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colourDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colourDesc.initialLayout = vk::ImageLayout::eUndefined;
	colourDesc.finalLayout = vk::ImageLayout::ePresentSrcKHR;
	vk::AttachmentReference colourRef;
	colourRef.attachment = 0;
	colourRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourRef;
	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colourDesc;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;
	return g_info.device.createRenderPass(createInfo);
}

vk::Pipeline vuk::createPipeline(vk::PipelineLayout layout, PipelineData const& data, vk::PipelineCache cache)
{
	auto const bindingDescription = Vertex::bindingDescription();
	auto const attributeDescriptions = Vertex::attributeDescriptions();
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
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	{
		depthStencilState.depthTestEnable = true;
		depthStencilState.depthWriteEnable = true;
		depthStencilState.depthCompareOp = vk::CompareOp::eLess;
	}
	std::array stateFlags = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamicState;
	{
		dynamicState.dynamicStateCount = (u32)stateFlags.size();
		dynamicState.pDynamicStates = stateFlags.data();
	}
	std::vector<vk::PipelineShaderStageCreateInfo> shaderCreateInfo;
	if (data.pShader)
	{
		auto modules = data.pShader->modules();
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
	createInfo.layout = layout;
	createInfo.renderPass = data.renderPass;
	createInfo.subpass = 0;
	return g_info.device.createGraphicsPipeline(cache, createInfo);
}
} // namespace le
