#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "resource_descriptors.hpp"
#include "resources.hpp"
#include "texture.hpp"

namespace le::gfx::rd
{
namespace
{
bool g_bSetLayoutsInit = false;

void writeSet(WriteInfo const& info)
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
	return;
}

void createLayouts()
{
	std::array const sceneBindings = {View::s_setLayoutBinding};
	std::array const objectBindings = {Flags::s_setLayoutBinding, Textures::s_setLayoutBinding};
	vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
	setLayoutCreateInfo.bindingCount = (u32)sceneBindings.size();
	setLayoutCreateInfo.pBindings = sceneBindings.data();
	rd::g_setLayouts.at((size_t)Type::eScene) = g_info.device.createDescriptorSetLayout(setLayoutCreateInfo);
	setLayoutCreateInfo.bindingCount = (u32)objectBindings.size();
	setLayoutCreateInfo.pBindings = objectBindings.data();
	rd::g_setLayouts.at((size_t)Type::eObject) = g_info.device.createDescriptorSetLayout(setLayoutCreateInfo);
	return;
}
} // namespace

void BufferWriter::writeBuffer(vk::DescriptorSet set, u32 binding, u32 size) const
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = size;
	WriteInfo writeInfo;
	writeInfo.set = set;
	writeInfo.binding = binding;
	writeInfo.pBuffer = &bufferInfo;
	writeInfo.type = vk::DescriptorType::eUniformBuffer;
	writeSet(writeInfo);
	return;
}

void TextureWriter::write(vk::DescriptorSet set, Texture const& texture, u32 binding)
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
	return;
}

vk::DescriptorSetLayoutBinding const View::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);

vk::DescriptorSetLayoutBinding const Flags::s_setLayoutBinding = vk::DescriptorSetLayoutBinding(
	0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

vk::DescriptorSetLayoutBinding const Textures::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

Sets::Sets()
{
	m_view.binding = View::s_setLayoutBinding.binding;
	m_flags.binding = Flags::s_setLayoutBinding.binding;
	m_diffuse.binding = Textures::s_setLayoutBinding.binding;
}

void Sets::writeView(View const& view)
{
	m_view.write(m_sets.at((size_t)Type::eScene), view);
	return;
}

void Sets::writeFlags(Flags const& flags)
{
	m_flags.write(m_sets.at((size_t)Type::eObject), flags);
	return;
}

void Sets::writeDiffuse(Texture const& diffuse)
{
	m_diffuse.write(m_sets.at((size_t)Type::eObject), diffuse);
	return;
}

void Sets::destroy()
{
	vram::release(m_view.writer.buffer, m_flags.writer.buffer);
	m_view.writer.buffer = m_flags.writer.buffer = {};
	return;
}

SetLayouts allocateSets(u32 copies)
{
	SetLayouts ret;
	// Pool of total descriptors
	vk::DescriptorPoolSize uboPoolSize;
	uboPoolSize.type = vk::DescriptorType::eUniformBuffer;
	uboPoolSize.descriptorCount = copies * 2;
	vk::DescriptorPoolSize sampler2DPoolSize;
	sampler2DPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
	sampler2DPoolSize.descriptorCount = copies * 1;
	std::array const poolSizes = {uboPoolSize, sampler2DPoolSize};
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = copies * 2; // 2 sets per copy
	ret.descriptorPool = g_info.device.createDescriptorPool(createInfo);
	// Allocate sets
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = ret.descriptorPool;
	allocInfo.descriptorSetCount = copies;
	std::vector<vk::DescriptorSetLayout> const sceneLayouts((size_t)copies, g_setLayouts.at((size_t)Type::eScene));
	allocInfo.pSetLayouts = sceneLayouts.data();
	auto const sceneSets = g_info.device.allocateDescriptorSets(allocInfo);
	std::vector<vk::DescriptorSetLayout> const objectLayouts((size_t)copies, g_setLayouts.at((size_t)Type::eObject));
	allocInfo.pSetLayouts = objectLayouts.data();
	auto const objectSets = g_info.device.allocateDescriptorSets(allocInfo);
	// Write handles
	ret.sets.reserve((size_t)copies);
	for (u32 idx = 0; idx < copies; ++idx)
	{
		Sets sets;
		sets.m_sets.at((size_t)Type::eScene) = sceneSets.at(idx);
		sets.m_sets.at((size_t)Type::eObject) = objectSets.at(idx);
		sets.writeView({});
		sets.writeFlags({});
		auto pTex = g_pResources->get<Texture>("textures/blank");
		ASSERT(pTex, "blank texture is null!");
		sets.writeDiffuse(*pTex);
		ret.sets.push_back(std::move(sets));
	}
	return ret;
}

void init()
{
	if (!g_bSetLayoutsInit)
	{
		createLayouts();
		g_bSetLayoutsInit = true;
	}
	return;
}

void deinit()
{
	if (g_bSetLayoutsInit)
	{
		for (auto& layout : g_setLayouts)
		{
			vkDestroy(layout);
			layout = vk::DescriptorSetLayout();
		}
		g_bSetLayoutsInit = false;
	}
}

void write(WriteInfo const& info)
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
} // namespace le::gfx::rd
