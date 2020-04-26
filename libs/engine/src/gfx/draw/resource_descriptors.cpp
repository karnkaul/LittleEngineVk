#include "core/log.hpp"
#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "resource_descriptors.hpp"
#include "engine/assets/resources.hpp"
#include "engine/gfx/texture.hpp"

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
	std::array const bindings = {View::s_setLayoutBinding,			SSBOModels::s_setLayoutBinding,	  SSBONormals::s_setLayoutBinding,
								 SSBOMaterials::s_setLayoutBinding, SSBOTints::s_setLayoutBinding,	  SSBOFlags::s_setLayoutBinding,
								 Textures::s_diffuseLayoutBinding,	Textures::s_specularLayoutBinding};
	vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
	setLayoutCreateInfo.bindingCount = (u32)bindings.size();
	setLayoutCreateInfo.pBindings = bindings.data();
	rd::g_setLayout = g_info.device.createDescriptorSetLayout(setLayoutCreateInfo);
	return;
}
} // namespace

SSBOMaterials::Mat::Mat(Material const& material)
	: ambient(material.m_albedo.ambient.toVec4()),
	  diffuse(material.m_albedo.diffuse.toVec4()),
	  specular(material.m_albedo.specular.toVec4()),
	  shininess(material.m_shininess)
{
}

vk::DescriptorSetLayoutBinding const View::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const SSBOModels::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const SSBONormals::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const SSBOMaterials::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const SSBOTints::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const SSBOFlags::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const Textures::s_diffuseLayoutBinding =
	vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eCombinedImageSampler, Textures::max, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const Textures::s_specularLayoutBinding =
	vk::DescriptorSetLayoutBinding(7, vk::DescriptorType::eCombinedImageSampler, Textures::max, vkFlags::vertFragShader);

void ShaderWriter::write(vk::DescriptorSet set, Buffer const& buffer, u32 idx) const
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = buffer.writeSize;
	WriteInfo writeInfo;
	writeInfo.set = set;
	writeInfo.binding = binding;
	writeInfo.arrayElement = idx;
	writeInfo.pBuffer = &bufferInfo;
	writeInfo.type = type;
	writeSet(writeInfo);
	return;
}

void ShaderWriter::write(vk::DescriptorSet set, Texture const& texture, u32 idx) const
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

void Set::create()
{
	m_view.create(vk::BufferUsageFlagBits::eUniformBuffer);
	m_models.create(vk::BufferUsageFlagBits::eStorageBuffer);
	m_normals.create(vk::BufferUsageFlagBits::eStorageBuffer);
	m_materials.create(vk::BufferUsageFlagBits::eStorageBuffer);
	m_tints.create(vk::BufferUsageFlagBits::eStorageBuffer);
	m_flags.create(vk::BufferUsageFlagBits::eStorageBuffer);
	m_view.writer.binding = View::s_setLayoutBinding.binding;
	m_view.writer.type = View::s_setLayoutBinding.descriptorType;
	m_models.writer.binding = SSBOModels::s_setLayoutBinding.binding;
	m_models.writer.type = SSBOModels::s_setLayoutBinding.descriptorType;
	m_normals.writer.binding = SSBONormals::s_setLayoutBinding.binding;
	m_normals.writer.type = SSBONormals::s_setLayoutBinding.descriptorType;
	m_materials.writer.binding = SSBOMaterials::s_setLayoutBinding.binding;
	m_materials.writer.type = SSBOMaterials::s_setLayoutBinding.descriptorType;
	m_tints.writer.binding = SSBOTints::s_setLayoutBinding.binding;
	m_tints.writer.type = SSBOTints::s_setLayoutBinding.descriptorType;
	m_flags.writer.binding = SSBOFlags::s_setLayoutBinding.binding;
	m_flags.writer.type = SSBOFlags::s_setLayoutBinding.descriptorType;
	m_diffuse.binding = Textures::s_diffuseLayoutBinding.binding;
	m_diffuse.type = Textures::s_diffuseLayoutBinding.descriptorType;
	m_specular.binding = Textures::s_specularLayoutBinding.binding;
	m_specular.type = Textures::s_diffuseLayoutBinding.descriptorType;
	return;
}

void Set::destroy()
{
	m_view.release();
	m_models.release();
	m_normals.release();
	m_materials.release();
	m_tints.release();
	m_flags.release();
	return;
}

void Set::writeView(View const& view)
{
	m_view.write(view, m_descriptorSet);
	return;
}

void Set::writeSSBOs(SSBOs const& ssbos)
{
	m_models.write(ssbos.models, m_descriptorSet);
	m_normals.write(ssbos.normals, m_descriptorSet);
	m_materials.write(ssbos.materials, m_descriptorSet);
	m_tints.write(ssbos.tints, m_descriptorSet);
	m_flags.write(ssbos.flags, m_descriptorSet);
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
	auto pBlack = g_pResources->get<Texture>("textures/black");
	auto pWhite = g_pResources->get<Texture>("textures/white");
	ASSERT(pBlack && pWhite, "blank textures are null!");
	for (u32 i = 0; i < Textures::max; ++i)
	{
		writeDiffuse(*pWhite, i);
		writeSpecular(*pBlack, i);
	}
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
	ssboPoolSize.descriptorCount = copies * 5; // 5 members per SSBO
	vk::DescriptorPoolSize sampler2DPoolSize;
	sampler2DPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
	sampler2DPoolSize.descriptorCount =
		copies * (Textures::s_diffuseLayoutBinding.descriptorCount + Textures::s_specularLayoutBinding.descriptorCount);
	std::array const poolSizes = {uboPoolSize, ssboPoolSize, sampler2DPoolSize};
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = copies; // 2 sets per copy
	ret.descriptorPool = g_info.device.createDescriptorPool(createInfo);
	// Allocate sets
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = ret.descriptorPool;
	allocInfo.descriptorSetCount = copies;
	std::vector<vk::DescriptorSetLayout> const setLayouts((size_t)copies, g_setLayout);
	allocInfo.pSetLayouts = setLayouts.data();
	auto const sets = g_info.device.allocateDescriptorSets(allocInfo);
	// Write handles
	ret.set.reserve((size_t)copies);
	for (u32 idx = 0; idx < copies; ++idx)
	{
		Set set;
		set.m_descriptorSet = sets.at(idx);
		set.create();
		set.writeView({});
		set.writeSSBOs({});
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
