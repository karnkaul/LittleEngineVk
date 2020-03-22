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

void vuk::writeUniformDescriptor(Buffer buffer, vk::DescriptorSet descriptorSet, u32 binding)
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = buffer.writeSize;
	vk::WriteDescriptorSet descWrite;
	descWrite.dstSet = descriptorSet;
	descWrite.dstBinding = binding;
	descWrite.dstArrayElement = 0;
	descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
	descWrite.descriptorCount = 1;
	descWrite.pBufferInfo = &bufferInfo;
	g_info.device.updateDescriptorSets(descWrite, {});
}

vuk::Image vuk::createImage(ImageData const& data)
{
	Image ret;
	vk::ImageCreateInfo imageInfo;
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
	VmaAllocationCreateInfo allocationInfo = {};
	allocationInfo.usage = data.vmaUsage;
	auto const vkImageInfo = static_cast<VkImageCreateInfo>(imageInfo);
	VkImage vkImage;
	if (vmaCreateImage(g_info.vmaAllocator, &vkImageInfo, &allocationInfo, &vkImage, &ret.handle, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Allocation error");
	}
	ret.image = vkImage;
	VmaAllocationInfo info;
	vmaGetAllocationInfo(g_info.vmaAllocator, ret.handle, &info);
	ret.info = {info.deviceMemory, info.offset, info.size};
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

vuk::Buffer vuk::createBuffer(BufferData const& data)
{
	Buffer ret;
	vk::BufferCreateInfo bufferInfo;
	ret.writeSize = bufferInfo.size = data.size;
	bufferInfo.usage = data.usage;
	auto const flags = Info::QFlags(Info::QFlag::eGraphics, Info::QFlag::eTransfer);
	bufferInfo.sharingMode = vuk::g_info.sharingMode(flags);
	auto const queueIndices = vuk::g_info.uniqueQueueIndices(flags);
	bufferInfo.queueFamilyIndexCount = (u32)queueIndices.size();
	bufferInfo.pQueueFamilyIndices = queueIndices.data();
	VmaAllocationCreateInfo allocationInfo = {};
	allocationInfo.usage = data.vmaUsage;
	auto const vkBufferInfo = static_cast<VkBufferCreateInfo>(bufferInfo);
	VkBuffer vkBuffer;
	if (vmaCreateBuffer(g_info.vmaAllocator, &vkBufferInfo, &allocationInfo, &vkBuffer, &ret.handle, nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Allocation error");
	}
	ret.buffer = vkBuffer;
	VmaAllocationInfo info;
	vmaGetAllocationInfo(g_info.vmaAllocator, ret.handle, &info);
	ret.info = {info.deviceMemory, info.offset, info.size};
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

bool vuk::writeToBuffer(Buffer buffer, void const* pData)
{
	if (buffer.info.memory != vk::DeviceMemory() && buffer.buffer != vk::Buffer())
	{
		auto pMem = g_info.device.mapMemory(buffer.info.memory, buffer.info.offset, buffer.writeSize);
		std::memcpy(pMem, pData, (size_t)buffer.writeSize);
		g_info.device.unmapMemory(buffer.info.memory);
		return true;
	}
	return false;
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
} // namespace le
