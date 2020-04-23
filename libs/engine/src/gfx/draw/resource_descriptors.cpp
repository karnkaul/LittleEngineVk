#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "resource_descriptors.hpp"
#include "engine/assets/resources.hpp"
#include "engine/gfx/draw/texture.hpp"

namespace le::gfx::rd
{
namespace
{
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
	std::array const bindings = {View::s_setLayoutBinding, Locals::s_setLayoutBinding, Textures::s_diffuseLayoutBinding,
								 Textures::s_specularLayoutBinding};
	vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
	setLayoutCreateInfo.bindingCount = (u32)bindings.size();
	setLayoutCreateInfo.pBindings = bindings.data();
	rd::g_setLayout = g_info.device.createDescriptorSetLayout(setLayoutCreateInfo);
	return;
}
} // namespace

vk::DescriptorSetLayoutBinding const View::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const Locals::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, Locals::max, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const Textures::s_diffuseLayoutBinding =
	vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, Textures::max, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const Textures::s_specularLayoutBinding =
	vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eCombinedImageSampler, Textures::max, vkFlags::vertFragShader);

void BufferWriter::writeBuffer(vk::DescriptorSet set, vk::DescriptorType type, u32 binding, u32 size, u32 idx) const
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = size;
	WriteInfo writeInfo;
	writeInfo.set = set;
	writeInfo.binding = binding;
	writeInfo.arrayElement = idx;
	writeInfo.pBuffer = &bufferInfo;
	writeInfo.type = type;
	writeSet(writeInfo);
	return;
}

void TextureWriter::write(vk::DescriptorSet set, vk::DescriptorType type, Texture const& texture, u32 binding, u32 idx)
{
	ASSERT(texture.m_pSampler, "Sampler is null!");
	vk::DescriptorImageInfo imageInfo;
	imageInfo.imageView = texture.m_uImpl->imageView;
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	imageInfo.sampler = texture.m_pSampler->m_uImpl->sampler;
	WriteInfo writeInfo;
	writeInfo.set = set;
	writeInfo.pImage = &imageInfo;
	writeInfo.binding = binding;
	writeInfo.arrayElement = idx;
	writeInfo.type = type;
	writeSet(writeInfo);
	return;
}

Set::Set()
{
	m_view.binding = View::s_setLayoutBinding.binding;
	m_view.type = vk::DescriptorType::eUniformBuffer;
	m_locals.binding = Locals::s_setLayoutBinding.binding;
	m_locals.type = vk::DescriptorType::eStorageBuffer;
	m_diffuse.binding = Textures::s_diffuseLayoutBinding.binding;
	m_diffuse.type = vk::DescriptorType::eCombinedImageSampler;
	m_specular.binding = Textures::s_specularLayoutBinding.binding;
	m_specular.type = vk::DescriptorType::eCombinedImageSampler;
}

void Set::writeView(View const& view)
{
	m_view.write(m_descriptorSet, view, 0U);
	return;
}

void Set::writeLocals(Locals const& flags, u32 idx)
{
	m_locals.write(m_descriptorSet, flags, idx);
	return;
}

void Set::writeDiffuse(Texture const& diffuse, u32 idx)
{
	m_diffuse.write(m_descriptorSet, diffuse, idx);
	return;
}

void Set::writeSpecular(Texture const& specular, u32 idx)
{
	m_specular.write(m_descriptorSet, specular, idx);
	return;
}

void Set::resetTextures()
{
	auto pTex = g_pResources->get<Texture>("textures/blank");
	ASSERT(pTex, "blank texture is null!");
	for (u32 i = 0; i < Textures::max; ++i)
	{
		writeDiffuse(*pTex, i);
		writeSpecular(*pTex, i);
	}
}

void Set::destroy()
{
	vram::release(m_view.writer.buffer, m_locals.writer.buffer);
	m_view.writer.buffer = m_locals.writer.buffer = {};
	return;
}

SetLayouts allocateSets(u32 copies)
{
	SetLayouts ret;
	// Pool of total descriptors
	vk::DescriptorPoolSize uboPoolSize;
	uboPoolSize.type = vk::DescriptorType::eUniformBuffer;
	uboPoolSize.descriptorCount = copies * View::s_setLayoutBinding.descriptorCount;
	vk::DescriptorPoolSize ssboPoolSize;
	ssboPoolSize.type = vk::DescriptorType::eStorageBuffer;
	ssboPoolSize.descriptorCount = copies * Locals::s_setLayoutBinding.descriptorCount;
	vk::DescriptorPoolSize sampler2DPoolSize;
	sampler2DPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
	sampler2DPoolSize.descriptorCount =
		copies * (Textures::s_diffuseLayoutBinding.descriptorCount + Textures::s_specularLayoutBinding.descriptorCount);
	std::array const poolSizes = {uboPoolSize, ssboPoolSize, sampler2DPoolSize};
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = copies * 2; // 2 sets per copy
	ret.descriptorPool = g_info.device.createDescriptorPool(createInfo);
	// Allocate sets
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = ret.descriptorPool;
	allocInfo.descriptorSetCount = copies;
	std::vector<vk::DescriptorSetLayout> const setLayouts((size_t)copies * 2, g_setLayout);
	allocInfo.pSetLayouts = setLayouts.data();
	auto const sets = g_info.device.allocateDescriptorSets(allocInfo);
	// Write handles
	ret.set.reserve((size_t)copies);
	for (u32 idx = 0; idx < copies; ++idx)
	{
		Set set;
		set.m_descriptorSet = sets.at(idx);
		set.writeView({});
		for (u32 i = 0; i < Locals::max; ++i)
		{
			set.writeLocals({}, i);
		}
		set.resetTextures();
		ret.set.push_back(std::move(set));
	}
	return ret;
}

void init()
{
	if (g_setLayout == vk::DescriptorSetLayout())
	{
		createLayouts();
	}
	return;
}

void deinit()
{
	if (g_setLayout != vk::DescriptorSetLayout())
	{
		vkDestroy(g_setLayout);
		g_setLayout = vk::DescriptorSetLayout();
	}
	return;
}
} // namespace le::gfx::rd
