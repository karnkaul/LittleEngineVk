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
	std::array const bindings = {UBOView::s_setLayoutBinding,		SSBOModels::s_setLayoutBinding,	  SSBONormals::s_setLayoutBinding,
								 SSBOMaterials::s_setLayoutBinding, SSBOTints::s_setLayoutBinding,	  SSBOFlags::s_setLayoutBinding,
								 SSBODirLights::s_setLayoutBinding, Textures::s_diffuseLayoutBinding, Textures::s_specularLayoutBinding,
								 Textures::s_cubemapLayoutBinding};
	vk::DescriptorSetLayoutCreateInfo setLayoutCreateInfo;
	setLayoutCreateInfo.bindingCount = (u32)bindings.size();
	setLayoutCreateInfo.pBindings = bindings.data();
	rd::g_setLayout = g_info.device.createDescriptorSetLayout(setLayoutCreateInfo);
	return;
}
} // namespace

UBOView::UBOView() = default;

UBOView::UBOView(Renderer::View const& view, u32 dirLightCount)
	: mat_vp(view.mat_vp), mat_v(view.mat_v), mat_p(view.mat_p), mat_ui(view.mat_ui), pos_v(view.pos_v), dirLightCount(dirLightCount)
{
}

SSBOMaterials::Mat::Mat(Material const& material, Colour dropColour)
	: ambient(material.m_albedo.ambient.toVec4()),
	  diffuse(material.m_albedo.diffuse.toVec4()),
	  specular(material.m_albedo.specular.toVec4()),
	  dropColour(dropColour.toVec4()),
	  shininess(material.m_shininess)
{
}

SSBODirLights::Light::Light(DirLight const& dirLight)
	: ambient(dirLight.ambient.toVec4()),
	  diffuse(dirLight.diffuse.toVec4()),
	  specular(dirLight.specular.toVec4()),
	  direction(dirLight.direction)
{
}

vk::DescriptorSetLayoutBinding const UBOView::s_setLayoutBinding =
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

vk::DescriptorSetLayoutBinding const SSBODirLights::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding Textures::s_diffuseLayoutBinding =
	vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eCombinedImageSampler, Textures::max, vkFlags::fragShader);

vk::DescriptorSetLayoutBinding Textures::s_specularLayoutBinding =
	vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eCombinedImageSampler, Textures::max, vkFlags::fragShader);

vk::DescriptorSetLayoutBinding const Textures::s_cubemapLayoutBinding =
	vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eCombinedImageSampler, 1, vkFlags::fragShader);

void Textures::clampDiffSpecCount(u32 hardwareMax)
{
	s_diffuseLayoutBinding.descriptorCount = std::min(hardwareMax, s_diffuseLayoutBinding.descriptorCount);
	s_specularLayoutBinding.descriptorCount = std::min(hardwareMax, s_specularLayoutBinding.descriptorCount);
}

u32 Textures::total()
{
	return s_diffuseLayoutBinding.descriptorCount + s_specularLayoutBinding.descriptorCount + s_cubemapLayoutBinding.descriptorCount;
}

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

void ShaderWriter::write(vk::DescriptorSet set, TextureImpl const& tex, u32 idx) const
{
	vk::DescriptorImageInfo imageInfo;
	imageInfo.imageView = tex.imageView;
	imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	imageInfo.sampler = tex.sampler;
	WriteInfo writeInfo;
	writeInfo.set = set;
	writeInfo.pImage = &imageInfo;
	writeInfo.binding = binding;
	writeInfo.arrayElement = idx;
	writeInfo.type = type;
	writeSet(writeInfo);
	return;
}

Set::Set() : m_view(vk::BufferUsageFlagBits::eUniformBuffer)
{
	m_diffuse.binding = Textures::s_diffuseLayoutBinding.binding;
	m_diffuse.type = Textures::s_diffuseLayoutBinding.descriptorType;
	m_specular.binding = Textures::s_specularLayoutBinding.binding;
	m_specular.type = Textures::s_diffuseLayoutBinding.descriptorType;
	m_cubemap.binding = Textures::s_cubemapLayoutBinding.binding;
	m_cubemap.type = Textures::s_cubemapLayoutBinding.descriptorType;
}

void Set::destroy()
{
	m_view.release();
	m_models.release();
	m_normals.release();
	m_materials.release();
	m_tints.release();
	m_flags.release();
	m_dirLights.release();
	return;
}

void Set::writeView(UBOView const& view)
{
	m_view.writeValue(view, m_descriptorSet);
	return;
}

void Set::initSSBOs()
{
	SSBOs ssbos;
	ssbos.models.ssbo.push_back({});
	ssbos.normals.ssbo.push_back({});
	ssbos.materials.ssbo.push_back({});
	ssbos.tints.ssbo.push_back({});
	ssbos.flags.ssbo.push_back({});
	ssbos.dirLights.ssbo.push_back({});
	writeSSBOs(ssbos);
}

void Set::writeSSBOs(SSBOs const& ssbos)
{
	ASSERT(!ssbos.models.ssbo.empty() && !ssbos.normals.ssbo.empty() && !ssbos.materials.ssbo.empty() && !ssbos.tints.ssbo.empty()
			   && !ssbos.flags.ssbo.empty(),
		   "Empty SSBOs!");
	m_models.writeArray(ssbos.models.ssbo, m_descriptorSet);
	m_normals.writeArray(ssbos.normals.ssbo, m_descriptorSet);
	m_materials.writeArray(ssbos.materials.ssbo, m_descriptorSet);
	m_tints.writeArray(ssbos.tints.ssbo, m_descriptorSet);
	m_flags.writeArray(ssbos.flags.ssbo, m_descriptorSet);
	if (!ssbos.dirLights.ssbo.empty())
	{
		m_dirLights.writeArray(ssbos.dirLights.ssbo, m_descriptorSet);
	}
	return;
}

void Set::writeDiffuse(Texture const& diffuse, u32 idx)
{
	m_diffuse.write(m_descriptorSet, *diffuse.m_uImpl, idx);
	return;
}

void Set::writeSpecular(Texture const& specular, u32 idx)
{
	m_specular.write(m_descriptorSet, *specular.m_uImpl, idx);
	return;
}

void Set::writeCubemap(Cubemap const& cubemap)
{
	m_cubemap.write(m_descriptorSet, *cubemap.m_uImpl, 0);
	return;
}

void Set::resetTextures()
{
	auto pBlack = Resources::inst().get<Texture>("textures/black");
	auto pWhite = Resources::inst().get<Texture>("textures/white");
	auto pCubemap = Resources::inst().get<Cubemap>("cubemaps/blank");
	ASSERT(pBlack && pWhite && pCubemap, "blank textures are null!");
	for (u32 i = 0; i < Textures::max; ++i)
	{
		writeDiffuse(*pWhite, i);
		writeSpecular(*pBlack, i);
	}
	writeCubemap(*pCubemap);
	return;
}

SetLayouts allocateSets(u32 copies)
{
	SetLayouts ret;
	// Pool of total descriptors
	vk::DescriptorPoolSize uboPoolSize;
	uboPoolSize.type = vk::DescriptorType::eUniformBuffer;
	uboPoolSize.descriptorCount = copies * UBOView::s_setLayoutBinding.descriptorCount;
	vk::DescriptorPoolSize ssboPoolSize;
	ssboPoolSize.type = vk::DescriptorType::eStorageBuffer;
	ssboPoolSize.descriptorCount = copies * 6; // 6 members per SSBO
	vk::DescriptorPoolSize samplerPoolSize;
	samplerPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
	samplerPoolSize.descriptorCount = copies * Textures::total();
	std::array const poolSizes = {uboPoolSize, ssboPoolSize, samplerPoolSize};
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = (u32)poolSizes.size();
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
		set.writeView({});
		set.initSSBOs();
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
