#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "resource_descriptors.hpp"
#include "resources.hpp"
#include "texture.hpp"

namespace le::gfx
{
namespace
{
bool g_bSetLayoutsInit = false;

void writeSet(rd::WriteInfo const& info)
{
	vk::WriteDescriptorSet descWrite;
	descWrite.dstSet = info.set;
	descWrite.dstBinding = info.binding;
	descWrite.dstArrayElement = info.arrayElement;
	descWrite.descriptorType = info.type;
	descWrite.descriptorCount = info.count;
	descWrite.pImageInfo = info.pImage;
	descWrite.pBufferInfo = info.pBuffer;
	g_info.device.updateDescriptorSets(descWrite, {});
}
} // namespace

vk::DescriptorSetLayoutBinding const ubo::View::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(binding, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);

vk::DescriptorSetLayoutBinding const ubo::Flags::s_setLayoutBinding = vk::DescriptorSetLayoutBinding(
	binding, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

void rd::BufferWriter::writeBuffer(vk::DescriptorSet set, u32 binding, u32 size) const
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = size;
	rd::WriteInfo writeInfo;
	writeInfo.set = set;
	writeInfo.binding = binding;
	writeInfo.pBuffer = &bufferInfo;
	writeInfo.type = vk::DescriptorType::eUniformBuffer;
	writeSet(writeInfo);
}

void rd::TextureWriter::write(vk::DescriptorSet set, Texture const& texture, u32 binding)
{
	ASSERT(texture.m_pSampler, "Sampler is null!");
	vk::DescriptorImageInfo imageInfo;
	imageInfo.imageView = texture.m_imageView;
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	imageInfo.sampler = texture.m_pSampler->m_sampler;
	WriteInfo writeInfo;
	writeInfo.set = set;
	writeInfo.pImage = &imageInfo;
	writeInfo.binding = binding;
	writeInfo.type = vk::DescriptorType::eCombinedImageSampler;
	writeSet(writeInfo);
}

void rd::Set::writeView(ubo::View const& view)
{
	m_view.write(m_set, view);
}

void rd::Set::writeFlags(ubo::Flags const& flags)
{
	m_flags.write(m_set, flags);
}

void rd::Set::writeDiffuse(Texture const& diffuse)
{
	m_diffuse.write(m_set, diffuse);
}

void rd::Set::destroy()
{
	vram::release(m_view.writer.buffer, m_flags.writer.buffer);
	m_view.writer.buffer = m_flags.writer.buffer = {};
}

rd::SetLayouts rd::SetLayouts::create()
{
	vk::DescriptorSetLayoutBinding sampler;
	sampler.binding = 2;
	sampler.descriptorCount = 1;
	sampler.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	sampler.stageFlags = vk::ShaderStageFlagBits::eFragment;
	std::array const bindings = {ubo::View::s_setLayoutBinding, ubo::Flags::s_setLayoutBinding, sampler};
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
	vk::DescriptorPoolSize uboPoolSize;
	uboPoolSize.type = vk::DescriptorType::eUniformBuffer;
	uboPoolSize.descriptorCount = descriptorSetCount * 2;
	vk::DescriptorPoolSize sampler2DPoolSize;
	sampler2DPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
	sampler2DPoolSize.descriptorCount = descriptorSetCount * 1;
	std::array const poolSizes = {uboPoolSize, sampler2DPoolSize};
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = descriptorSetCount;
	ret.descriptorPool = g_info.device.createDescriptorPool(createInfo);
	// Allocate sets
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = ret.descriptorPool;
	allocInfo.descriptorSetCount = descriptorSetCount;
	std::vector<vk::DescriptorSetLayout> const uboLayouts((size_t)descriptorSetCount, layouts.at((size_t)Type::eUniformBuffer));
	allocInfo.pSetLayouts = uboLayouts.data();
	auto const uboSets = g_info.device.allocateDescriptorSets(allocInfo);
	// Write handles
	ret.sets.reserve((size_t)descriptorSetCount);
	for (u32 idx = 0; idx < descriptorSetCount; ++idx)
	{
		Set set;
		set.m_set = uboSets.at(idx);
		set.writeView({});
		set.writeFlags({});
		auto pTex = g_pResources->get<Texture>("textures/blank");
		ASSERT(pTex, "blank texture is null!");
		set.writeDiffuse(*pTex);
		ret.sets.push_back(set);
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

void rd::write(WriteInfo const& info)
{
	vk::WriteDescriptorSet descWrite;
	descWrite.dstSet = info.set;
	descWrite.dstBinding = info.binding;
	descWrite.dstArrayElement = info.arrayElement;
	descWrite.descriptorType = info.type;
	descWrite.descriptorCount = info.count;
	descWrite.pImageInfo = info.pImage;
	descWrite.pBufferInfo = info.pBuffer;
	g_info.device.updateDescriptorSets(descWrite, {});
}
} // namespace le::gfx
