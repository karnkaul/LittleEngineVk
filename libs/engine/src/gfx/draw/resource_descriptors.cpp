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
	std::array const globals = {View::s_setLayoutBinding};
	std::array const locals = {Locals::s_setLayoutBinding, Textures::s_diffuseLayoutBinding, Textures::s_specularLayoutBinding};
	vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
	setLayoutCreateInfo.bindingCount = (u32)globals.size();
	setLayoutCreateInfo.pBindings = globals.data();
	rd::g_setLayouts.at((size_t)Type::eScene) = g_info.device.createDescriptorSetLayout(setLayoutCreateInfo);
	setLayoutCreateInfo.bindingCount = (u32)locals.size();
	setLayoutCreateInfo.pBindings = locals.data();
	rd::g_setLayouts.at((size_t)Type::eObject) = g_info.device.createDescriptorSetLayout(setLayoutCreateInfo);
	return;
}
} // namespace

vk::DescriptorSetLayoutBinding const View::s_setLayoutBinding = vk::DescriptorSetLayoutBinding(
	0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

vk::DescriptorSetLayoutBinding const Locals::s_setLayoutBinding = vk::DescriptorSetLayoutBinding(
	0, vk::DescriptorType::eStorageBuffer, Locals::max, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

vk::DescriptorSetLayoutBinding const Textures::s_diffuseLayoutBinding =
	vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, Textures::max, vk::ShaderStageFlagBits::eFragment);

vk::DescriptorSetLayoutBinding const Textures::s_specularLayoutBinding =
	vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, Textures::max, vk::ShaderStageFlagBits::eFragment);

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
	imageInfo.imageView = texture.m_imageView;
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	imageInfo.sampler = texture.m_pSampler->m_sampler;
	WriteInfo writeInfo;
	writeInfo.set = set;
	writeInfo.pImage = &imageInfo;
	writeInfo.binding = binding;
	writeInfo.arrayElement = idx;
	writeInfo.type = type;
	writeSet(writeInfo);
	return;
}

Sets::Sets()
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

void Sets::writeView(View const& view)
{
	m_view.write(m_sets.at((size_t)Type::eScene), view, 0U);
	return;
}

void Sets::writeLocals(Locals const& flags, u32 idx)
{
	m_locals.write(m_sets.at((size_t)Type::eObject), flags, idx);
	return;
}

void Sets::writeDiffuse(Texture const& diffuse, u32 idx)
{
	m_diffuse.write(m_sets.at((size_t)Type::eObject), diffuse, idx);
	return;
}

void Sets::writeSpecular(Texture const& specular, u32 idx)
{
	m_specular.write(m_sets.at((size_t)Type::eObject), specular, idx);
	return;
}

void Sets::resetTextures()
{
	auto pTex = g_pResources->get<Texture>("textures/blank");
	ASSERT(pTex, "blank texture is null!");
	for (u32 i = 0; i < Textures::max; ++i)
	{
		writeDiffuse(*pTex, i);
		writeSpecular(*pTex, i);
	}
}

void Sets::destroy()
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
		for (u32 i = 0; i < Locals::max; ++i)
		{
			sets.writeLocals({}, i);
		}
		sets.resetTextures();
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
} // namespace le::gfx::rd
