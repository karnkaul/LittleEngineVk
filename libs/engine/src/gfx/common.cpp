#include "engine/gfx/mesh.hpp"
#include "common.hpp"
#include "device.hpp"

namespace le
{
namespace gfx
{
std::unordered_map<vk::Result, std::string_view> g_vkResultStr = {
	{vk::Result::eErrorOutOfHostMemory, "OutOfHostMemory"},
	{vk::Result::eErrorOutOfDeviceMemory, "OutOfDeviceMemory"},
	{vk::Result::eSuccess, "Success"},
	{vk::Result::eSuboptimalKHR, "SubmoptimalSurface"},
	{vk::Result::eErrorDeviceLost, "DeviceLost"},
	{vk::Result::eErrorSurfaceLostKHR, "SurfaceLost"},
	{vk::Result::eErrorFullScreenExclusiveModeLostEXT, "FullScreenExclusiveModeLost"},
	{vk::Result::eErrorOutOfDateKHR, "OutOfDateSurface"},
};

vk::ShaderModule ShaderImpl::module(Shader::Type type) const
{
	ASSERT(shaders.at((size_t)type) != vk::ShaderModule(), "Module not present in Shader!");
	return shaders.at((size_t)type);
}

std::array<vk::ShaderStageFlagBits, size_t(Shader::Type::eCOUNT_)> const ShaderImpl::s_typeToFlagBit = {vk::ShaderStageFlagBits::eVertex,
																										vk::ShaderStageFlagBits::eFragment};

std::map<Shader::Type, vk::ShaderModule> ShaderImpl::modules() const
{
	std::map<Shader::Type, vk::ShaderModule> ret;
	for (size_t idx = 0; idx < (size_t)Shader::Type::eCOUNT_; ++idx)
	{
		auto const& module = shaders.at(idx);
		if (module != vk::ShaderModule())
		{
			ret[(Shader::Type)idx] = module;
		}
	}
	return ret;
}

vk::VertexInputBindingDescription vbo::bindingDescription()
{
	vk::VertexInputBindingDescription ret;
	ret.binding = vertexBinding;
	ret.stride = sizeof(Vertex);
	ret.inputRate = vk::VertexInputRate::eVertex;
	return ret;
}

std::vector<vk::VertexInputAttributeDescription> vbo::attributeDescriptions()
{
	std::vector<vk::VertexInputAttributeDescription> ret;
	vk::VertexInputAttributeDescription pos;
	pos.binding = vertexBinding;
	pos.location = 0;
	pos.format = vk::Format::eR32G32B32Sfloat;
	pos.offset = offsetof(Vertex, position);
	ret.push_back(pos);
	vk::VertexInputAttributeDescription col;
	col.binding = vertexBinding;
	col.location = 1;
	col.format = vk::Format::eR32G32B32Sfloat;
	col.offset = offsetof(Vertex, colour);
	ret.push_back(col);
	vk::VertexInputAttributeDescription norm;
	norm.binding = vertexBinding;
	norm.location = 2;
	norm.format = vk::Format::eR32G32B32Sfloat;
	norm.offset = offsetof(Vertex, normal);
	ret.push_back(norm);
	vk::VertexInputAttributeDescription uv;
	uv.binding = vertexBinding;
	uv.location = 3;
	uv.format = vk::Format::eR32G32Sfloat;
	uv.offset = offsetof(Vertex, texCoord);
	ret.push_back(uv);
	return ret;
}
} // namespace gfx

vk::PipelineLayout gfx::createPipelineLayout(vk::ArrayProxy<vk::PushConstantRange const> pushConstants,
											 vk::ArrayProxy<vk::DescriptorSetLayout const> setLayouts)
{
	vk::PipelineLayoutCreateInfo createInfo;
	createInfo.setLayoutCount = setLayouts.size();
	createInfo.pSetLayouts = setLayouts.data();
	createInfo.pushConstantRangeCount = pushConstants.size();
	createInfo.pPushConstantRanges = pushConstants.data();
	return g_device.device.createPipelineLayout(createInfo);
}

vk::DescriptorSetLayout gfx::createDescriptorSetLayout(vk::ArrayProxy<vk::DescriptorSetLayoutBinding const> bindings)
{
	vk::DescriptorSetLayoutCreateInfo createInfo;
	createInfo.bindingCount = bindings.size();
	createInfo.pBindings = bindings.data();
	return g_device.device.createDescriptorSetLayout(createInfo);
}

vk::DescriptorPool gfx::createDescriptorPool(vk::ArrayProxy<vk::DescriptorPoolSize const> poolSizes, u32 maxSets)
{
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = maxSets;
	return g_device.device.createDescriptorPool(createInfo);
}

vk::RenderPass gfx::createRenderPass(vk::ArrayProxy<vk::AttachmentDescription const> attachments,
									 vk::ArrayProxy<vk::SubpassDescription const> subpasses,
									 vk::ArrayProxy<vk::SubpassDependency> dependencies)
{
	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = attachments.size();
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = subpasses.size();
	createInfo.pSubpasses = subpasses.data();
	createInfo.dependencyCount = dependencies.size();
	createInfo.pDependencies = dependencies.data();
	return g_device.device.createRenderPass(createInfo);
}
} // namespace le
