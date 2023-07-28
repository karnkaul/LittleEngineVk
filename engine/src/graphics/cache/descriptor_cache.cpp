#include <spaced/error.hpp>
#include <spaced/graphics/cache/descriptor_cache.hpp>
#include <spaced/graphics/device.hpp>
#include <spaced/graphics/renderer.hpp>

namespace spaced::graphics {
namespace {
auto make_descriptor_pool(vk::Device device) -> vk::UniqueDescriptorPool {
	static constexpr std::uint32_t descriptor_count_v{3};
	static constexpr std::uint32_t max_sets_v{3};

	auto const pool_sizes = std::array{
		vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, descriptor_count_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, descriptor_count_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, descriptor_count_v},
	};

	auto dpci = vk::DescriptorPoolCreateInfo{};
	dpci.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	dpci.maxSets = max_sets_v;
	dpci.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
	dpci.pPoolSizes = pool_sizes.data();
	return device.createDescriptorPoolUnique(dpci);
}
} // namespace

auto DescriptorCache::try_allocate(vk::DescriptorSetLayout const& layout, vk::DescriptorSet& out) const -> bool {
	auto const& data = m_data[Renderer::self().frame_index()];
	if (!data.active) { return false; }
	auto dsai = vk::DescriptorSetAllocateInfo{};
	dsai.descriptorPool = *data.active;
	dsai.pSetLayouts = &layout;
	dsai.descriptorSetCount = 1;
	return Device::self().device().allocateDescriptorSets(&dsai, &out) == vk::Result::eSuccess;
}

auto DescriptorCache::allocate(vk::DescriptorSetLayout const& layout) -> vk::DescriptorSet {
	static constexpr auto max_loops{5};
	auto const device = Device::self().device();
	auto ret = vk::DescriptorSet{};
	auto& data = m_data[Renderer::self().frame_index()];
	for (int i = 0; i < max_loops; ++i) {
		if (try_allocate(layout, ret)) { return ret; }
		if (data.active) { data.used.push_back(std::move(data.active)); }
		if (data.free.empty()) { data.free.push_back(make_descriptor_pool(device)); }
		data.active = std::move(data.free.back());
		data.free.pop_back();
	}
	throw Error{"Failed to allocate Vulkan Descriptor Set"};
}

auto DescriptorCache::next_frame() -> void {
	auto const device = Device::self().device();
	auto& data = m_data[Renderer::self().frame_index()];
	if (data.active) { data.used.push_back(std::move(data.active)); }
	for (auto& pool : data.used) { device.resetDescriptorPool(*pool); }
	std::ranges::move(data.used, std::back_inserter(data.free));
	data.used.clear();
	if (data.free.empty()) { data.free.push_back(make_descriptor_pool(device)); }
	data.active = std::move(data.free.back());
	data.free.pop_back();
}

auto DescriptorCache::clear() -> void { m_data = {}; }
} // namespace spaced::graphics
