#include <core/log.hpp>
#include <gfx/device.hpp>
#include <gfx/vram.hpp>
#include <gfx/resource_descriptors.hpp>
#include <engine/resources/resources.hpp>
#include <resources/resources_impl.hpp>

namespace le::gfx
{
namespace rd
{
std::vector<vk::VertexInputBindingDescription> vbo::vertexBindings()
{
	vk::VertexInputBindingDescription ret;
	ret.binding = vertexBinding;
	ret.stride = sizeof(Vertex);
	ret.inputRate = vk::VertexInputRate::eVertex;
	return {ret};
}

std::vector<vk::VertexInputAttributeDescription> vbo::vertexAttributes()
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

View::View() = default;

View::View(Renderer::View const& view, u32 dirLightCount)
	: mat_vp(view.mat_vp), mat_v(view.mat_v), mat_p(view.mat_p), mat_ui(view.mat_ui), pos_v(view.pos_v), dirLightCount(dirLightCount)
{
}

Materials::Mat::Mat(res::Material::Info const& material, Colour dropColour)
	: ambient(material.albedo.ambient.toVec4()),
	  diffuse(material.albedo.diffuse.toVec4()),
	  specular(material.albedo.specular.toVec4()),
	  dropColour(dropColour.toVec4()),
	  shininess(material.shininess)
{
}

DirLights::Light::Light(DirLight const& dirLight)
	: ambient(dirLight.ambient.toVec4()), diffuse(dirLight.diffuse.toVec4()), specular(dirLight.specular.toVec4()), direction(dirLight.direction)
{
}

u32 ImageSamplers::s_max = 1024;

vk::DescriptorSetLayoutBinding const View::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const Models::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const Normals::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const Materials::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const Tints::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const Flags::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(5, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding const DirLights::s_setLayoutBinding =
	vk::DescriptorSetLayoutBinding(6, vk::DescriptorType::eStorageBuffer, 1, vkFlags::vertFragShader);

vk::DescriptorSetLayoutBinding ImageSamplers::s_diffuseLayoutBinding =
	vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, s_max, vkFlags::fragShader);

vk::DescriptorSetLayoutBinding ImageSamplers::s_specularLayoutBinding =
	vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, s_max, vkFlags::fragShader);

vk::DescriptorSetLayoutBinding const ImageSamplers::s_cubemapLayoutBinding =
	vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, vkFlags::fragShader);

u32 ImageSamplers::total()
{
	return s_diffuseLayoutBinding.descriptorCount + s_specularLayoutBinding.descriptorCount + s_cubemapLayoutBinding.descriptorCount;
}

void ImageSamplers::clampDiffSpecCount(u32 hardwareMax)
{
	s_max = std::min(s_max, (hardwareMax - 1) / 2); // (total - cubemap) / (diffuse + specular)
	s_diffuseLayoutBinding.descriptorCount = s_specularLayoutBinding.descriptorCount = s_max;
}

std::vector<vk::PushConstantRange> PushConstants::ranges()
{
	vk::PushConstantRange pcRange;
	pcRange.size = sizeof(PushConstants);
	pcRange.stageFlags = vkFlags::vertFragShader;
	return {pcRange};
}

void Writer::write(vk::DescriptorSet set, Buffer const& buffer) const
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer.buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = buffer.writeSize;
	vk::WriteDescriptorSet descWrite;
	descWrite.dstSet = set;
	descWrite.dstBinding = binding;
	descWrite.dstArrayElement = 0;
	descWrite.descriptorType = type;
	descWrite.descriptorCount = 1;
	descWrite.pBufferInfo = &bufferInfo;
	g_device.device.updateDescriptorSets(descWrite, {});
	return;
}

void Writer::write(vk::DescriptorSet set, std::vector<res::Texture> const& textures) const
{
	std::vector<vk::DescriptorImageInfo> imageInfos;
	imageInfos.reserve(textures.size());
	for (auto const& tex : textures)
	{
		if (auto pImpl = res::impl(tex))
		{
			auto const& info = tex.info();
			if (auto pSamplerImpl = res::impl(info.sampler))
			{
				vk::DescriptorImageInfo imageInfo;
				imageInfo.imageView = pImpl->imageView;
				imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				imageInfo.sampler = pSamplerImpl->sampler;
				imageInfos.push_back(imageInfo);
			}
		}
	}
	vk::WriteDescriptorSet descWrite;
	descWrite.dstSet = set;
	descWrite.dstBinding = binding;
	descWrite.dstArrayElement = 0;
	descWrite.descriptorType = type;
	descWrite.descriptorCount = (u32)imageInfos.size();
	descWrite.pImageInfo = imageInfos.data();
	g_device.device.updateDescriptorSets(descWrite, {});
	return;
}

void Descriptor<ImageSamplers>::writeArray(std::vector<res::Texture> const& textures, vk::DescriptorSet set) const
{
	m_writer.write(set, textures);
}

Set::Set() : m_view(vk::BufferUsageFlagBits::eUniformBuffer)
{
	m_diffuse.m_writer.binding = ImageSamplers::s_diffuseLayoutBinding.binding;
	m_diffuse.m_writer.type = ImageSamplers::s_diffuseLayoutBinding.descriptorType;
	m_specular.m_writer.binding = ImageSamplers::s_specularLayoutBinding.binding;
	m_specular.m_writer.type = ImageSamplers::s_diffuseLayoutBinding.descriptorType;
	m_cubemap.m_writer.binding = ImageSamplers::s_cubemapLayoutBinding.binding;
	m_cubemap.m_writer.type = ImageSamplers::s_cubemapLayoutBinding.descriptorType;
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

void Set::resetTextures(SamplerCounts const& counts)
{
	auto [white, bWhite] = res::find<res::Texture>("textures/white");
	auto [black, bBlack] = res::find<res::Texture>("textures/black");
	ASSERT(bBlack && bWhite, "Default textures missing!");
	std::deque<res::Texture> const diffuse((std::size_t)counts.diffuse, white);
	std::deque<res::Texture> const specular((std::size_t)counts.specular, black);
	writeDiffuse(diffuse);
	writeSpecular(specular);
}

void Set::writeView(View const& view)
{
	m_view.writeValue(view, m_bufferSet);
	return;
}

void Set::initSSBOs()
{
	StorageBuffers ssbos;
	ssbos.models.ssbo.push_back({});
	ssbos.normals.ssbo.push_back({});
	ssbos.materials.ssbo.push_back({});
	ssbos.tints.ssbo.push_back({});
	ssbos.flags.ssbo.push_back({});
	ssbos.dirLights.ssbo.push_back({});
	writeSSBOs(ssbos);
}

void Set::writeSSBOs(StorageBuffers const& ssbos)
{
	ASSERT(!ssbos.models.ssbo.empty() && !ssbos.normals.ssbo.empty() && !ssbos.materials.ssbo.empty() && !ssbos.tints.ssbo.empty() && !ssbos.flags.ssbo.empty(),
		   "Empty SSBOs!");
	m_models.writeArray(ssbos.models.ssbo, m_bufferSet);
	m_normals.writeArray(ssbos.normals.ssbo, m_bufferSet);
	m_materials.writeArray(ssbos.materials.ssbo, m_bufferSet);
	m_tints.writeArray(ssbos.tints.ssbo, m_bufferSet);
	m_flags.writeArray(ssbos.flags.ssbo, m_bufferSet);
	if (!ssbos.dirLights.ssbo.empty())
	{
		m_dirLights.writeArray(ssbos.dirLights.ssbo, m_bufferSet);
	}
	return;
}

void Set::writeDiffuse(std::deque<res::Texture> const& diffuse)
{
	std::vector<res::Texture> arr;
	std::copy(diffuse.begin(), diffuse.end(), std::back_inserter(arr));
	m_diffuse.writeArray(arr, m_samplerSet);
	return;
}

void Set::writeSpecular(std::deque<res::Texture> const& specular)
{
	std::vector<res::Texture> arr;
	std::copy(specular.begin(), specular.end(), std::back_inserter(arr));
	m_specular.writeArray(arr, m_samplerSet);
	return;
}

void Set::writeCubemap(res::Texture cubemap)
{
	m_cubemap.writeArray({cubemap}, m_samplerSet);
	return;
}
} // namespace rd

void rd::init()
{
	if (g_bufferLayout == vk::DescriptorSetLayout())
	{
		std::array const bufferBindings = {View::s_setLayoutBinding,  Models::s_setLayoutBinding, Normals::s_setLayoutBinding,	Materials::s_setLayoutBinding,
										   Tints::s_setLayoutBinding, Flags::s_setLayoutBinding,  DirLights::s_setLayoutBinding};
		vk::DescriptorSetLayoutCreateInfo bufferLayoutInfo;
		bufferLayoutInfo.bindingCount = (u32)bufferBindings.size();
		bufferLayoutInfo.pBindings = bufferBindings.data();
		rd::g_bufferLayout = g_device.device.createDescriptorSetLayout(bufferLayoutInfo);
	}
	return;
}

void rd::deinit()
{
	if (g_bufferLayout != vk::DescriptorSetLayout())
	{
		g_device.destroy(g_bufferLayout);
		g_bufferLayout = vk::DescriptorSetLayout();
	}
	return;
}

rd::SetLayouts rd::allocateSets(u32 copies, SamplerCounts const& samplerCounts)
{
	SetLayouts ret;
	auto diffuseBinding = ImageSamplers::s_diffuseLayoutBinding;
	diffuseBinding.descriptorCount = samplerCounts.diffuse;
	auto specularBinding = ImageSamplers::s_specularLayoutBinding;
	specularBinding.descriptorCount = samplerCounts.specular;
	std::array const samplerBindings = {diffuseBinding, specularBinding, ImageSamplers::s_cubemapLayoutBinding};
	ret.samplerLayout = g_device.createDescriptorSetLayout(samplerBindings);
	ret.sets.reserve((std::size_t)copies);
	for (u32 idx = 0; idx < copies; ++idx)
	{
		Set set;
		// Pool of descriptors
		vk::DescriptorPoolSize uboPoolSize;
		uboPoolSize.type = vk::DescriptorType::eUniformBuffer;
		uboPoolSize.descriptorCount = View::s_setLayoutBinding.descriptorCount;
		vk::DescriptorPoolSize ssboPoolSize;
		ssboPoolSize.type = vk::DescriptorType::eStorageBuffer;
		ssboPoolSize.descriptorCount = 6; // 6 members per SSBO
		vk::DescriptorPoolSize samplerPoolSize;
		samplerPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
		samplerPoolSize.descriptorCount = ImageSamplers::total();
		std::array const bufferPoolSizes = {uboPoolSize, ssboPoolSize, samplerPoolSize};
		std::array const samplerPoolSizes = {samplerPoolSize};
		set.m_bufferPool = g_device.createDescriptorPool(bufferPoolSizes);
		set.m_samplerPool = g_device.createDescriptorPool(samplerPoolSizes);
		// Allocate sets
		set.m_bufferSet = g_device.allocateDescriptorSets(set.m_bufferPool, g_bufferLayout).front();
		set.m_samplerSet = g_device.allocateDescriptorSets(set.m_samplerPool, ret.samplerLayout).front();
		// Write handles
		set.writeView({});
		set.initSSBOs();
		set.resetTextures(samplerCounts);
		ret.sets.push_back(std::move(set));
	}
	return ret;
}

vk::RenderPass rd::createSingleRenderPass(vk::Format colour, vk::Format depth)
{
	std::array<vk::AttachmentDescription, 2> attachments;
	vk::AttachmentReference colourAttachment, depthAttachment;
	{
		attachments.at(0).format = colour;
		attachments.at(0).samples = vk::SampleCountFlagBits::e1;
		attachments.at(0).loadOp = vk::AttachmentLoadOp::eClear;
		attachments.at(0).stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachments.at(0).stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachments.at(0).initialLayout = vk::ImageLayout::eUndefined;
		attachments.at(0).finalLayout = vk::ImageLayout::ePresentSrcKHR;
		colourAttachment.attachment = 0;
		colourAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
	}
	{
		attachments.at(1).format = depth;
		attachments.at(1).samples = vk::SampleCountFlagBits::e1;
		attachments.at(1).loadOp = vk::AttachmentLoadOp::eClear;
		attachments.at(1).storeOp = vk::AttachmentStoreOp::eDontCare;
		attachments.at(1).stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachments.at(1).stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachments.at(1).initialLayout = vk::ImageLayout::eUndefined;
		attachments.at(1).finalLayout = vk::ImageLayout::ePresentSrcKHR;
		depthAttachment.attachment = 1;
		depthAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	}
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachment;
	subpass.pDepthStencilAttachment = &depthAttachment;
	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	return g_device.createRenderPass(attachments, subpass, dependency);
}
} // namespace le::gfx
