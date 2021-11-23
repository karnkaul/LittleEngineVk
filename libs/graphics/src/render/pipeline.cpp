#include <core/maths.hpp>
#include <core/utils/expect.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/command_buffer.hpp>
#include <graphics/render/pipeline.hpp>
#include <graphics/shader.hpp>
#include <graphics/utils/utils.hpp>
#include <vulkan/vulkan.hpp>
#include <algorithm>

namespace le::graphics {
namespace {
template <typename T, typename U>
void ensureSet(T& out_src, U&& u) {
	if (out_src == T()) { out_src = u; }
}

template <typename T, typename U>
void setIfSet(T& out_dst, U&& u) {
	if (!Device::default_v(u)) { out_dst = u; }
}
} // namespace

Pipeline::Pipeline(not_null<VRAM*> vram, Shader const& shader, CreateInfo info, Hash id) : m_vram(vram), m_device(vram->m_device) {
	m_metadata.main = std::move(info);
	m_storage.id = id;
	vk::Pipeline pipe;
	if (setup(shader) && construct(m_metadata.main, pipe)) { m_storage.dynamic.main = {m_device, pipe}; }
}

Pipeline::Pipeline(not_null<VRAM*> vram, Shader const& shader, graphics::PipelineSpec const& state, vk::RenderPass rp, Hash id)
	: m_vram(vram), m_device(vram->m_device) {
	m_storage.id = id;
	if (setup(shader)) {
		utils::PipeData data;
		data.renderPass = rp;
		data.layout = layout();
		if (auto pipe = utils::makeGraphicsPipeline(*m_device, shader.m_modules, state, data)) { m_storage.dynamic.main = {m_device, *pipe}; }
	}
	// vk::Pipeline pipe;
	// if (setup(shader) && construct(m_metadata.main, pipe)) { m_storage.dynamic.main = {m_device, pipe}; }
}

std::optional<vk::Pipeline> Pipeline::constructVariant(Hash id, CreateInfo::Fixed const& fixed) {
	if (id == Hash()) { return std::nullopt; }
	vk::Pipeline pipe;
	CreateInfo info = m_metadata.main;
	info.fixedState = fixed;
	if (!construct(info, pipe)) { return std::nullopt; }
	m_metadata.variants[id] = std::move(info.fixedState);
	m_storage.dynamic.variants[id] = {m_device, pipe};
	return pipe;
}

std::optional<vk::Pipeline> Pipeline::variant(Hash id) const {
	if (id == Hash()) { return m_storage.dynamic.main; }
	if (auto it = m_storage.dynamic.variants.find(id); it != m_storage.dynamic.variants.end()) { return it->second; }
	return std::nullopt;
}

bool Pipeline::reconstruct(Shader const& shader) {
	vk::Pipeline pipe;
	auto info = m_metadata.main;
	if (!setup(shader) || !construct(info, pipe)) { return false; }
	m_storage.dynamic.main = {m_device, pipe};
	m_metadata.name = shader.m_name;
	m_metadata.main = std::move(info);
	for (auto& [id, f] : m_metadata.variants) {
		vk::Pipeline pipe;
		auto ci = m_metadata.main;
		ci.fixedState = f;
		if (!construct(ci, pipe)) { return false; }
		m_storage.dynamic.variants[id] = {m_device, pipe};
	}
	return true;
}

vk::PipelineBindPoint Pipeline::bindPoint() const { return m_metadata.main.bindPoint; }

vk::PipelineLayout Pipeline::layout() const { return m_storage.fixed.layout; }

ShaderInput& Pipeline::shaderInput() { return m_storage.input; }

ShaderInput const& Pipeline::shaderInput() const { return m_storage.input; }

DescriptorPool Pipeline::makeSetPool(u32 set, Buffering buffering) const {
	ENSURE(set < (u32)m_storage.fixed.setLayouts.size(), "Set does not exist on pipeline!");
	auto& f = m_storage.fixed;
	if (buffering == 0_B) { buffering = m_metadata.main.buffering; }
	DescriptorSet::CreateInfo const info{m_metadata.name, f.setLayouts[(std::size_t)set], f.bindingInfos[(std::size_t)set], buffering, set};
	return DescriptorPool(m_device, info);
}

std::unordered_map<u32, DescriptorPool> Pipeline::makeSetPools(Buffering buffering) const {
	std::unordered_map<u32, DescriptorPool> ret;
	auto const& f = m_storage.fixed;
	if (buffering == 0_B) { buffering = m_metadata.main.buffering; }
	for (u32 set = 0; set < (u32)m_storage.fixed.setLayouts.size(); ++set) {
		DescriptorSet::CreateInfo const info{m_metadata.name, f.setLayouts[(std::size_t)set], f.bindingInfos[(std::size_t)set], buffering, set};
		ret.emplace(set, DescriptorPool(m_device, info));
	}
	return ret;
}

void Pipeline::bindSet(CommandBuffer cb, u32 set, std::size_t idx) const {
	if (m_storage.input.contains(set)) { cb.bindSet(m_storage.fixed.layout, m_storage.input.pool(set).index(idx)); }
}

void Pipeline::bindSet(CommandBuffer cb, std::initializer_list<u32> sets, std::size_t idx) const {
	for (u32 const set : sets) { bindSet(cb, set, idx); }
}

Pipeline::CreateInfo::Fixed Pipeline::fixedState(Hash variant) const noexcept {
	if (variant == Hash()) { return m_metadata.main.fixedState; }
	if (auto it = m_metadata.variants.find(variant); it != m_metadata.variants.end()) { return it->second; }
	return {};
}

bool Pipeline::setup(Shader const& shader) {
	EXPECT(utils::hasActiveModule(shader.m_modules));
	if (!utils::hasActiveModule(shader.m_modules)) { return false; }
	auto& f = m_storage.fixed;
	f = {};
	auto setBindings = utils::extractBindings(shader.m_spirV);
	std::vector<vk::DescriptorSetLayout> layouts;
	for (auto& [set, binds] : setBindings.sets) {
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		for (auto& setBinding : binds) {
			if (setBinding.binding.descriptorType != vk::DescriptorType()) { bindings.push_back(setBinding.binding); }
		}
		auto const descLayout = m_device->makeDescriptorSetLayout(bindings);
		f.setLayouts.push_back({m_device, descLayout});
		f.bindingInfos.push_back(std::move(binds));
		layouts.push_back(descLayout);
	}
	f.layout = makeDeferred<vk::PipelineLayout>(m_device, setBindings.push, layouts);
	m_metadata.name = shader.m_name;
	m_storage.input = ShaderInput(*this, m_metadata.main.buffering);
	m_storage.dynamic.modules = shader.m_modules;
	return true;
}

bool Pipeline::construct(CreateInfo& out_info, vk::Pipeline& out_pipe) {
	PipelineSpec spec;
	out_info.fixedState.dynamicStates.insert(vk::DynamicState::eViewport);
	out_info.fixedState.dynamicStates.insert(vk::DynamicState::eScissor);
	spec.fixedState.dynamicStates = {out_info.fixedState.dynamicStates.begin(), out_info.fixedState.dynamicStates.end()};
	if (out_info.fixedState.depthStencilState.depthTestEnable) { spec.fixedState.flags.set(PFlag::eDepthTest); }
	if (out_info.fixedState.depthStencilState.depthWriteEnable) { spec.fixedState.flags.set(PFlag::eDepthWrite); }
	if (out_info.fixedState.colorBlendAttachment.blendEnable) { spec.fixedState.flags.set(PFlag::eAlphaBlend); }
	if (out_info.fixedState.rasterizerState.polygonMode == vk::PolygonMode::eLine) { spec.fixedState.flags.set(PFlag::eWireframe); }
	spec.fixedState.lineWidth = out_info.fixedState.rasterizerState.lineWidth;
	spec.fixedState.mode = out_info.fixedState.rasterizerState.polygonMode;
	spec.fixedState.topology = out_info.fixedState.topology;
	spec.subpass = out_info.subpass;
	spec.vertexInput = m_metadata.main.fixedState.vertexInput;
	utils::PipeData data;
	data.layout = m_storage.fixed.layout;
	data.renderPass = m_metadata.main.renderPass;
	data.cache = out_info.cache;
	if (auto pipe = utils::makeGraphicsPipeline(*m_device, m_storage.dynamic.modules, spec, data)) {
		out_pipe = *pipe;
		return true;
	}
	return false;
}
} // namespace le::graphics
