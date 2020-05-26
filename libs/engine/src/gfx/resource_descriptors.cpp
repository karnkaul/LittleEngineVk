#include "core/log.hpp"
#include "device.hpp"
#include "vram.hpp"
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
	g_device.device.updateDescriptorSets(descWrite, {});
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
	rd::g_setLayout = g_device.device.createDescriptorSetLayout(setLayoutCreateInfo);
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
	vk::DescriptorSetLayoutBinding(10, vk::DescriptorType::eCombinedImageSampler, max, vkFlags::fragShader);

vk::DescriptorSetLayoutBinding Textures::s_specularLayoutBinding =
	vk::DescriptorSetLayoutBinding(11, vk::DescriptorType::eCombinedImageSampler, max, vkFlags::fragShader);

vk::DescriptorSetLayoutBinding const Textures::s_cubemapLayoutBinding =
	vk::DescriptorSetLayoutBinding(12, vk::DescriptorType::eCombinedImageSampler, 1, vkFlags::fragShader);

u32 Textures::total()
{
	return s_diffuseLayoutBinding.descriptorCount + s_specularLayoutBinding.descriptorCount + s_cubemapLayoutBinding.descriptorCount;
}

void Textures::clampDiffSpecCount(u32 hardwareMax)
{
	u32 const diffSpecMax = (hardwareMax - 1) / 2; // (total - cubemap) / (diffuse + specular)
	s_diffuseLayoutBinding.descriptorCount = std::min(s_diffuseLayoutBinding.descriptorCount, diffSpecMax);
	s_specularLayoutBinding.descriptorCount = std::min(s_specularLayoutBinding.descriptorCount, diffSpecMax);
}

void ShaderWriter::write(vk::DescriptorSet set, Buffer const& buffer) const
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = buffer.writeSize;
	WriteInfo writeInfo;
	writeInfo.set = set;
	writeInfo.binding = binding;
	writeInfo.arrayElement = 0;
	writeInfo.pBuffer = &bufferInfo;
	writeInfo.type = type;
	writeSet(writeInfo);
	return;
}

void ShaderWriter::write(vk::DescriptorSet set, std::vector<TextureImpl const*> const& textures) const
{
	std::vector<vk::DescriptorImageInfo> imageInfos;
	imageInfos.reserve(textures.size());
	for (auto pTex : textures)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageView = pTex->imageView;
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.sampler = pTex->sampler;
		imageInfos.push_back(imageInfo);
	}
	WriteInfo writeInfo;
	writeInfo.set = set;
	writeInfo.pImage = imageInfos.data();
	writeInfo.binding = binding;
	writeInfo.arrayElement = 0;
	writeInfo.count = (u32)imageInfos.size();
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
	m_view.writeValue(view, m_set);
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
	m_models.writeArray(ssbos.models.ssbo, m_set);
	m_normals.writeArray(ssbos.normals.ssbo, m_set);
	m_materials.writeArray(ssbos.materials.ssbo, m_set);
	m_tints.writeArray(ssbos.tints.ssbo, m_set);
	m_flags.writeArray(ssbos.flags.ssbo, m_set);
	if (!ssbos.dirLights.ssbo.empty())
	{
		m_dirLights.writeArray(ssbos.dirLights.ssbo, m_set);
	}
	return;
}

void Set::writeDiffuse(std::deque<Texture const*> const& diffuse)
{
	std::vector<TextureImpl const*> diffuseImpl;
	diffuseImpl.reserve(diffuse.size());
	for (auto pTex : diffuse)
	{
		diffuseImpl.push_back(pTex->m_uImpl.get());
	}
	m_diffuse.write(m_set, diffuseImpl);
	return;
}

void Set::writeSpecular(std::deque<Texture const*> const& specular)
{
	std::vector<TextureImpl const*> specularImpl;
	specularImpl.reserve(specular.size());
	for (auto pTex : specular)
	{
		specularImpl.push_back(pTex->m_uImpl.get());
	}
	m_specular.write(m_set, specularImpl);
	return;
}

void Set::writeCubemap(Cubemap const& cubemap)
{
	m_cubemap.write(m_set, {cubemap.m_uImpl.get()});
	return;
}

void Set::resetTextures()
{
	auto pBlack = Resources::inst().get<Texture>("textures/black");
	auto pWhite = Resources::inst().get<Texture>("textures/white");
	auto pCubemap = Resources::inst().get<Cubemap>("cubemaps/blank");
	ASSERT(pBlack && pWhite && pCubemap, "blank textures are null!");
	std::deque<Texture const*> diffuse(Textures::s_diffuseLayoutBinding.descriptorCount, pWhite);
	std::deque<Texture const*> specular(Textures::s_specularLayoutBinding.descriptorCount, pBlack);
	writeDiffuse(diffuse);
	writeSpecular(specular);
	writeCubemap(*pCubemap);
	return;
}

std::vector<Set> allocateSets(vk::DescriptorSetLayout layout, u32 copies)
{
	std::vector<Set> ret;
	ret.reserve((size_t)copies);
	for (u32 idx = 0; idx < copies; ++idx)
	{
		Set set;
		// Pool of descriptors
		vk::DescriptorPoolSize uboPoolSize;
		uboPoolSize.type = vk::DescriptorType::eUniformBuffer;
		uboPoolSize.descriptorCount = UBOView::s_setLayoutBinding.descriptorCount;
		vk::DescriptorPoolSize ssboPoolSize;
		ssboPoolSize.type = vk::DescriptorType::eStorageBuffer;
		ssboPoolSize.descriptorCount = 6; // 6 members per SSBO
		vk::DescriptorPoolSize samplerPoolSize;
		samplerPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
		samplerPoolSize.descriptorCount = Textures::total();
		std::array const poolSizes = {uboPoolSize, ssboPoolSize, samplerPoolSize};
		vk::DescriptorPoolCreateInfo createInfo;
		createInfo.poolSizeCount = (u32)poolSizes.size();
		createInfo.pPoolSizes = poolSizes.data();
		createInfo.maxSets = 1;
		set.m_pool = g_device.device.createDescriptorPool(createInfo);
		// Allocate sets
		vk::DescriptorSetAllocateInfo allocInfo;
		allocInfo.descriptorPool = set.m_pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;
		auto const sets = g_device.device.allocateDescriptorSets(allocInfo);
		// Write handles
		set.m_set = sets.front();
		set.writeView({});
		set.initSSBOs();
		set.resetTextures();
		ret.push_back(std::move(set));
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
		g_device.destroy(g_setLayout);
		g_setLayout = vk::DescriptorSetLayout();
	}
	return;
}
} // namespace le::gfx::rd
