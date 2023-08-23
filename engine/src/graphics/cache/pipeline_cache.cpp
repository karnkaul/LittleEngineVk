#include <le/core/hash_combine.hpp>
#include <le/core/logger.hpp>
#include <le/graphics/cache/pipeline_cache.hpp>
#include <le/graphics/device.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <algorithm>
#include <map>

namespace le::graphics {
namespace {
struct PipelineShaderLayout {
	std::vector<vk::UniqueDescriptorSetLayout> descriptor_set_layouts{};
	std::vector<vk::DescriptorSetLayout> descriptor_set_layouts_view{};

	static auto make(ShaderLayout const& shader_layout, vk::Device device) -> PipelineShaderLayout {
		auto ordered_set_layouts = shader_layout.custom_sets;

		auto& camera_set = ordered_set_layouts[shader_layout.camera.set];
		camera_set.emplace_back(shader_layout.camera.view, vk::DescriptorType::eUniformBuffer, 1);
		camera_set.emplace_back(shader_layout.camera.directional_lights, vk::DescriptorType::eStorageBuffer, 1);
		camera_set.emplace_back(shader_layout.camera.shadow_map, vk::DescriptorType::eCombinedImageSampler, 1);

		auto& material_set = ordered_set_layouts[shader_layout.material.set];
		material_set.emplace_back(shader_layout.material.data, vk::DescriptorType::eUniformBuffer, 1);
		for (auto const texture_binding : shader_layout.material.textures) {
			material_set.emplace_back(texture_binding, vk::DescriptorType::eCombinedImageSampler, 1);
		}

		auto& object_set = ordered_set_layouts[shader_layout.object.set];
		object_set.emplace_back(shader_layout.object.instances, vk::DescriptorType::eStorageBuffer, 1);
		object_set.emplace_back(shader_layout.object.joints, vk::DescriptorType::eStorageBuffer, 1);

		for (auto& [_, bindings] : ordered_set_layouts) {
			for (auto& binding : bindings) { binding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment; }
		}

		auto ret = PipelineShaderLayout{};
		for (auto const& [set, bindings] : ordered_set_layouts) {
			auto dslci = vk::DescriptorSetLayoutCreateInfo{};
			dslci.bindingCount = static_cast<std::uint32_t>(bindings.size());
			dslci.pBindings = bindings.data();
			ret.descriptor_set_layouts.push_back(device.createDescriptorSetLayoutUnique(dslci));
			ret.descriptor_set_layouts_view.push_back(*ret.descriptor_set_layouts.back());
		}
		return ret;
	}
};

auto const g_log{logger::Logger{"Cache"}};
} // namespace

PipelineCache::Key::Key(PipelineFormat format, Shader shader, PipelineState state, vk::PolygonMode polygon_mode)
	: format(format), shader(std::move(shader)), state(state), polygon_mode(polygon_mode) {
	cached_hash = make_combined_hash(shader.vertex.hash(), shader.fragment.hash(), state.topology, polygon_mode, state.depth_compare, state.depth_test_write,
									 format.colour, format.depth);
}

PipelineCache::PipelineCache(ShaderLayout shader_layout) { set_shader_layout(std::move(shader_layout)); }

auto PipelineCache::set_shader_layout(ShaderLayout shader_layout) -> void {
	m_shader_layout = std::move(shader_layout);

	m_device = Device::self().get_device();
	auto pipeline_shader_layout = PipelineShaderLayout::make(m_shader_layout, m_device);

	m_descriptor_set_layouts = std::move(pipeline_shader_layout).descriptor_set_layouts;
	m_descriptor_set_layouts_view = std::move(pipeline_shader_layout.descriptor_set_layouts_view);

	auto plci = vk::PipelineLayoutCreateInfo{};
	plci.setLayoutCount = static_cast<std::uint32_t>(m_descriptor_set_layouts_view.size());
	plci.pSetLayouts = m_descriptor_set_layouts_view.data();
	m_pipeline_layout = m_device.createPipelineLayoutUnique(plci);
}

auto PipelineCache::load(PipelineFormat format, Shader shader, PipelineState state, vk::PolygonMode polygon_mode) -> vk::Pipeline {
	auto const key = Key{format, std::move(shader), state, polygon_mode};
	auto itr = m_pipelines.find(key);
	if (itr == m_pipelines.end()) {
		auto ret = build(key);
		if (!ret) { return {}; }
		auto const [inserted, _] = m_pipelines.insert_or_assign(key, std::move(ret));
		itr = inserted;

		g_log.debug("new Vulkan Pipeline created [{}] (total: {})", key.hash(), m_pipelines.size());
	}
	assert(itr != m_pipelines.end());
	return *itr->second;
}

auto PipelineCache::clear_pipelines() -> void {
	g_log.debug("[{}] Vulkan Pipelines destroyed", m_pipelines.size());
	m_pipelines.clear();
}

auto PipelineCache::clear_pipelines_and_shaders() -> void {
	m_device.waitIdle();
	clear_pipelines();
	m_shader_cache.clear_shaders();
}

auto PipelineCache::build(Key const& key) -> vk::UniquePipeline {
	auto shader_stages = std::array<vk::PipelineShaderStageCreateInfo, 2>{};
	shader_stages[0].stage = vk::ShaderStageFlagBits::eVertex;
	shader_stages[1].stage = vk::ShaderStageFlagBits::eFragment;
	shader_stages[0].pName = shader_stages[1].pName = "main";

	auto vertex_shader = m_shader_cache.load(key.shader.vertex);
	auto fragment_shader = m_shader_cache.load(key.shader.fragment);
	if (vertex_shader == nullptr || fragment_shader == nullptr) { return {}; }

	shader_stages[0].module = vertex_shader;
	shader_stages[1].module = fragment_shader;
	assert(shader_stages[0].module && shader_stages[1].module);

	auto pvisci = vk::PipelineVertexInputStateCreateInfo{};
	pvisci.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(m_shader_layout.vertex_layout.attributes.size());
	pvisci.pVertexAttributeDescriptions = m_shader_layout.vertex_layout.attributes.data();
	pvisci.vertexBindingDescriptionCount = static_cast<std::uint32_t>(m_shader_layout.vertex_layout.bindings.size());
	pvisci.pVertexBindingDescriptions = m_shader_layout.vertex_layout.bindings.data();

	auto gpci = vk::GraphicsPipelineCreateInfo{};
	gpci.pVertexInputState = &pvisci;
	gpci.stageCount = static_cast<std::uint32_t>(shader_stages.size());
	gpci.pStages = shader_stages.data();

	auto prsci = vk::PipelineRasterizationStateCreateInfo{};
	prsci.polygonMode = key.polygon_mode;
	prsci.cullMode = vk::CullModeFlagBits::eNone;
	gpci.pRasterizationState = &prsci;

	auto const piasci = vk::PipelineInputAssemblyStateCreateInfo{{}, key.state.topology};
	gpci.pInputAssemblyState = &piasci;

	auto pdssci = vk::PipelineDepthStencilStateCreateInfo{};
	pdssci.depthTestEnable = pdssci.depthWriteEnable = key.state.depth_test_write;
	pdssci.depthCompareOp = key.state.depth_compare;
	gpci.pDepthStencilState = &pdssci;

	auto pcbas = vk::PipelineColorBlendAttachmentState{};
	using CCF = vk::ColorComponentFlagBits;
	pcbas.colorWriteMask = CCF::eR | CCF::eG | CCF::eB | CCF::eA;
	pcbas.blendEnable = 1;
	pcbas.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	pcbas.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	pcbas.colorBlendOp = vk::BlendOp::eAdd;
	pcbas.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	pcbas.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	pcbas.alphaBlendOp = vk::BlendOp::eAdd;
	auto pcbsci = vk::PipelineColorBlendStateCreateInfo();
	pcbsci.attachmentCount = 1;
	pcbsci.pAttachments = &pcbas;
	gpci.pColorBlendState = &pcbsci;

	auto pdsci = vk::PipelineDynamicStateCreateInfo();
	auto const pdscis = std::array{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
		vk::DynamicState::eLineWidth,
	};
	pdsci = vk::PipelineDynamicStateCreateInfo({}, static_cast<std::uint32_t>(pdscis.size()), pdscis.data());
	gpci.pDynamicState = &pdsci;

	auto pvsci = vk::PipelineViewportStateCreateInfo({}, 1, {}, 1);
	gpci.pViewportState = &pvsci;

	auto pmsci = vk::PipelineMultisampleStateCreateInfo{};
	pmsci.rasterizationSamples = vk::SampleCountFlagBits::e1;
	pmsci.sampleShadingEnable = 0;
	gpci.pMultisampleState = &pmsci;

	auto prci = vk::PipelineRenderingCreateInfo{};
	if (key.format.colour != vk::Format{}) {
		prci.colorAttachmentCount = 1u;
		prci.pColorAttachmentFormats = &key.format.colour;
	}
	prci.depthAttachmentFormat = key.format.depth;
	gpci.pNext = &prci;

	gpci.layout = *m_pipeline_layout;
	auto ret = vk::Pipeline{};
	if (m_device.createGraphicsPipelines({}, 1, &gpci, {}, &ret) != vk::Result::eSuccess) { return {}; }

	return vk::UniquePipeline{ret, m_device};
}
} // namespace le::graphics
