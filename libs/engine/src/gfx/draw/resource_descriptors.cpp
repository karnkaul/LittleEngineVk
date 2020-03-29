#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "resource_descriptors.hpp"

namespace le::gfx
{
namespace
{
bool g_bSetLayoutsInit = false;
}

vk::DescriptorSetLayoutBinding const ubo::View::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(binding, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);

vk::DescriptorSetLayoutBinding const ubo::Flags::s_setLayoutBinding = vk::DescriptorSetLayoutBinding(
	binding, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

void rd::Handles::release()
{
	vram::release(view.buffer, flags.buffer);
	view = {};
	flags = {};
}

rd::SetLayouts rd::SetLayouts::create()
{
	std::array const bindings = {ubo::View::s_setLayoutBinding, ubo::Flags::s_setLayoutBinding};
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
	// Pool of total descriptors
	vk::DescriptorPoolSize descPoolSize;
	descPoolSize.type = vk::DescriptorType::eUniformBuffer;
	descPoolSize.descriptorCount = descriptorSetCount * descriptorCount;
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &descPoolSize;
	createInfo.maxSets = descriptorSetCount;
	ret.pool = g_info.device.createDescriptorPool(createInfo);
	// Allocate sets
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = ret.pool;
	allocInfo.descriptorSetCount = descriptorSetCount;
	std::vector<vk::DescriptorSetLayout> const uboLayouts((size_t)descriptorSetCount, layouts.at((size_t)Type::eUniformBuffer));
	allocInfo.pSetLayouts = uboLayouts.data();
	auto const sets = g_info.device.allocateDescriptorSets(allocInfo);
	// Write handles
	ret.descriptorHandles.reserve((size_t)descriptorSetCount);
	for (u32 idx = 0; idx < descriptorSetCount; ++idx)
	{
		Handles handles;
		handles.view = ubo::Handle<ubo::View>::create(sets.at(idx));
		handles.flags = ubo::Handle<ubo::Flags>::create(sets.at(idx));
		ret.descriptorHandles.push_back(handles);
	}
	return ret;
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
