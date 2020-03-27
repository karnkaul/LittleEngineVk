#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "resource_descriptors.hpp"

namespace le::gfx
{
namespace
{
bool g_bSetLayoutsInit = false;
}

rd::SetLayouts rd::SetLayouts::create()
{
	vk::DescriptorSetLayoutBinding viewInfo;
	viewInfo.binding = ubo::View::binding;
	viewInfo.descriptorCount = 1;
	viewInfo.stageFlags = vk::ShaderStageFlagBits::eVertex;
	viewInfo.descriptorType = vk::DescriptorType::eUniformBuffer;
	vk::DescriptorSetLayoutBinding flagsInfo;
	flagsInfo.binding = ubo::Flags::binding;
	flagsInfo.descriptorCount = 1;
	flagsInfo.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
	flagsInfo.descriptorType = vk::DescriptorType::eUniformBuffer;
	std::vector<vk::DescriptorSetLayoutBinding> const bindings = {viewInfo, flagsInfo};
	vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
	setLayoutCreateInfo.bindingCount = (u32)bindings.size();
	setLayoutCreateInfo.pBindings = bindings.data();
	SetLayouts ret;
	ret.layouts.at((size_t)Type::eUniformBuffer) = g_info.device.createDescriptorSetLayout(setLayoutCreateInfo);
	ret.descriptorCount = (u32)bindings.size();
	return ret;
}

rd::Setup rd::SetLayouts::allocateSets(u32 descriptorSetCount)
{
	Setup ret;
	vk::DescriptorPoolSize descPoolSize;
	descPoolSize.type = vk::DescriptorType::eUniformBuffer;
	descPoolSize.descriptorCount = descriptorSetCount * descriptorCount;
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &descPoolSize;
	createInfo.maxSets = descriptorSetCount;
	ret.pool = g_info.device.createDescriptorPool(createInfo);
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = ret.pool;
	allocInfo.descriptorSetCount = descriptorSetCount;
	std::vector const uboLayouts((size_t)descriptorSetCount, layouts.at((size_t)Type::eUniformBuffer));
	allocInfo.pSetLayouts = uboLayouts.data();
	auto const sets = g_info.device.allocateDescriptorSets(allocInfo);
	ret.descriptorHandles.reserve((size_t)descriptorSetCount);
	for (u32 idx = 0; idx < descriptorSetCount; ++idx)
	{
		Handles handles;
		handles.view = ubo::Handle<ubo::View>::create(layouts.at((size_t)Type::eUniformBuffer), sets.at(idx));
		handles.flags = ubo::Handle<ubo::Flags>::create(layouts.at((size_t)Type::eUniformBuffer), sets.at(idx));
		ret.descriptorHandles.push_back(handles);
	}
	return ret;
}

void rd::Handles::release()
{
	vram::release(view.buffer, flags.buffer);
	view = {};
	flags = {};
}

void rd::init()
{
	if (!g_bSetLayoutsInit)
	{
		g_setLayouts = SetLayouts::create();
		g_bSetLayoutsInit = true;
	}
	return;
}

void rd::deinit()
{
	if (g_bSetLayoutsInit)
	{
		for (auto& layout : g_setLayouts.layouts)
		{
			vkDestroy(layout);
		}
		g_setLayouts = SetLayouts();
		g_bSetLayoutsInit = false;
	}
}
} // namespace le::gfx
