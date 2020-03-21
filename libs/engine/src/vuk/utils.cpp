#include <set>
#include "info.hpp"
#include "utils.hpp"
#include "draw/shader.hpp"
#include "draw/vertex.hpp"

namespace le
{
TResult<vk::Format> vuk::supportedFormat(PriorityList<vk::Format> const& desired, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
	for (auto format : desired)
	{
		vk::FormatProperties props = g_info.physicalDevice.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	return {};
}

void vuk::wait(vk::Fence optional)
{
	if (optional != vk::Fence())
	{
		g_info.device.waitForFences(optional, true, maxVal<u64>());
	}
}

void vuk::waitAll(vk::ArrayProxy<const vk::Fence> validFences)
{
	g_info.device.waitForFences(std::move(validFences), true, maxVal<u64>());
}

vk::DescriptorSetLayout vuk::createDescriptorSetLayout(u32 binding, u32 descriptorCount, vk::ShaderStageFlags stages)
{
	vk::DescriptorSetLayoutBinding setLayoutBinding;
	setLayoutBinding.binding = binding;
	setLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	setLayoutBinding.descriptorCount = descriptorCount;
	setLayoutBinding.stageFlags = stages;
	vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
	setLayoutCreateInfo.bindingCount = 1;
	setLayoutCreateInfo.pBindings = &setLayoutBinding;
	return g_info.device.createDescriptorSetLayout(setLayoutCreateInfo);
}

void vuk::writeUniformDescriptor(VkResource<vk::Buffer> buffer, vk::DescriptorSet descriptorSet, u32 binding)
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer.resource;
	bufferInfo.offset = 0;
	bufferInfo.range = buffer.alloc.size;
	vk::WriteDescriptorSet descWrite;
	descWrite.dstSet = descriptorSet;
	descWrite.dstBinding = binding;
	descWrite.dstArrayElement = 0;
	descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
	descWrite.descriptorCount = 1;
	descWrite.pBufferInfo = &bufferInfo;
	g_info.device.updateDescriptorSets(descWrite, {});
}

vuk::VkResource<vk::Image> vuk::createImage(ImageData const& data)
{
	VkResource<vk::Image> ret;
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
	auto const queueIndices = g_info.uniqueQueueIndices(data.queueFlags);
	imageInfo.queueFamilyIndexCount = (u32)queueIndices.size();
	imageInfo.pQueueFamilyIndices = queueIndices.data();
	imageInfo.sharingMode = g_info.sharingMode(data.queueFlags);
	ret.resource = g_info.device.createImage(imageInfo);
	vk::MemoryRequirements memRequirements = g_info.device.getImageMemoryRequirements(ret.resource);
	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = g_info.findMemoryType(memRequirements.memoryTypeBits, data.properties);
	ret.alloc.memory = g_info.device.allocateMemory(allocInfo);
	ret.alloc.size = memRequirements.size;
	g_info.device.bindImageMemory(ret.resource, ret.alloc.memory, 0);
	return ret;
}

vk::ImageView vuk::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, vk::ImageViewType type)
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
	VkResource<vk::Buffer> ret;
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = data.size;
	bufferInfo.usage = data.usage;
	auto const flags = Info::QFlags(Info::QFlag::eGraphics, Info::QFlag::eTransfer);
	bufferInfo.sharingMode = vuk::g_info.sharingMode(flags);
	auto const queueIndices = vuk::g_info.uniqueQueueIndices(flags);
	bufferInfo.queueFamilyIndexCount = (u32)queueIndices.size();
	bufferInfo.pQueueFamilyIndices = queueIndices.data();
	ret.resource = vuk::g_info.device.createBuffer(bufferInfo);
	vk::MemoryRequirements memRequirements = vuk::g_info.device.getBufferMemoryRequirements(ret.resource);
	vk::MemoryAllocateInfo allocInfo = {};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = vuk::g_info.findMemoryType(memRequirements.memoryTypeBits, data.properties);
	ret.alloc.memory = vuk::g_info.device.allocateMemory(allocInfo);
	ret.alloc.size = data.size;
	vuk::g_info.device.bindBufferMemory(ret.resource, ret.alloc.memory, ret.alloc.offset);
	return ret;
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
		rasterizerState.lineWidth = data.staticLineWidth;
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
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;
	}
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	{
		depthStencilState.depthTestEnable = true;
		depthStencilState.depthWriteEnable = true;
		depthStencilState.depthCompareOp = vk::CompareOp::eLess;
	}
	auto states = data.dynamicStates;
	states.insert(vk::DynamicState::eViewport);
	states.insert(vk::DynamicState::eScissor);
	std::vector<vk::DynamicState> stateFlags = {states.begin(), states.end()};
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

bool vuk::writeToBuffer(VkResource<vk::Buffer> buffer, void const* pData)
{
	if (buffer.alloc.memory != vk::DeviceMemory() && buffer.resource != vk::Buffer())
	{
		auto pMem = g_info.device.mapMemory(buffer.alloc.memory, buffer.alloc.offset, buffer.alloc.size);
		std::memcpy(pMem, pData, buffer.alloc.size);
		g_info.device.unmapMemory(buffer.alloc.memory);
		return true;
	}
	return false;
}
} // namespace le
