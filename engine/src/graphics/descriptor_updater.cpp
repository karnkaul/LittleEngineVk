#include <spaced/graphics/cache/descriptor_cache.hpp>
#include <spaced/graphics/cache/sampler_cache.hpp>
#include <spaced/graphics/cache/scratch_buffer_cache.hpp>
#include <spaced/graphics/descriptor_updater.hpp>
#include <spaced/graphics/device.hpp>

namespace spaced::graphics {
namespace {
template <typename T>
auto update_set(T const& info, vk::Device device, vk::DescriptorSet set, vk::DescriptorType type, std::uint32_t binding, std::uint32_t count) -> void {
	auto wds = vk::WriteDescriptorSet{};
	wds.descriptorCount = count;
	wds.descriptorType = type;
	wds.dstSet = set;
	wds.dstBinding = binding;
	if constexpr (std::same_as<T, vk::DescriptorBufferInfo>) {
		wds.pBufferInfo = &info;
	} else {
		wds.pImageInfo = &info;
	}
	device.updateDescriptorSets(wds, {});
}
} // namespace

DescriptorUpdater::DescriptorUpdater(std::uint32_t const set) : m_set(set) {
	auto const layouts = PipelineCache::self().descriptor_set_layouts();
	if (set >= layouts.size()) { throw Error{"Set number out of range"}; }
	m_descriptor_set = DescriptorCache::self().allocate(layouts[set]);
}

// NOLINTNEXTLINE
auto DescriptorUpdater::shader_layout() const -> ShaderLayout const& { return PipelineCache::self().shader_layout(); }

auto DescriptorUpdater::update(std::uint32_t binding, vk::DescriptorType type, vk::DescriptorBufferInfo const& info, std::uint32_t count)
	-> DescriptorUpdater& {
	update_set(info, Device::self().device(), m_descriptor_set, type, binding, count);
	return *this;
}

auto DescriptorUpdater::update(std::uint32_t binding, vk::DescriptorImageInfo const& info, std::uint32_t count) -> DescriptorUpdater& {
	update_set(info, Device::self().device(), m_descriptor_set, vk::DescriptorType::eCombinedImageSampler, binding, count);
	return *this;
}

auto DescriptorUpdater::write_uniform(std::uint32_t binding, void const* data, std::size_t size, vk::DeviceSize count) -> DescriptorUpdater& {
	return write(binding, vk::DescriptorType::eUniformBuffer, data, size, count);
}

auto DescriptorUpdater::write_storage(std::uint32_t binding, void const* data, std::size_t size, vk::DeviceSize count) -> DescriptorUpdater& {
	return write(binding, vk::DescriptorType::eStorageBuffer, data, size, count);
}

auto DescriptorUpdater::update_texture(std::uint32_t binding, Image const& texture, TextureSampler const& sampler) -> DescriptorUpdater& {
	auto const vk_sampler = SamplerCache::self().get(sampler);
	if (!vk_sampler) { throw Error{"Failed to create Vulkan Sampler"}; }
	return update(binding, vk::DescriptorImageInfo{vk_sampler, texture, vk::ImageLayout::eReadOnlyOptimal}, 1);
}

auto DescriptorUpdater::update_texture(std::uint32_t binding, Texture const& texture) -> DescriptorUpdater& {
	return update_texture(binding, texture.image(), texture.sampler);
}

auto DescriptorUpdater::write(std::uint32_t binding, vk::DescriptorType type, void const* data, std::size_t size, vk::DeviceSize count) -> DescriptorUpdater& {
	auto const usage = type == vk::DescriptorType::eStorageBuffer ? vk::BufferUsageFlagBits::eStorageBuffer : vk::BufferUsageFlagBits::eUniformBuffer;
	if (data == nullptr || size == 0) {
		auto const& empty = ScratchBufferCache::self().get_empty_buffer(usage);
		return update(binding, type, vk::DescriptorBufferInfo{empty.buffer(), {}, empty.size()}, count);
	}

	auto& buffer = ScratchBufferCache::self().allocate_host(usage);
	buffer.write(data, size);
	return update(binding, type, vk::DescriptorBufferInfo{buffer.buffer(), {}, buffer.size()}, count);
}

auto DescriptorUpdater::bind_set(vk::CommandBuffer cmd) const -> void {
	auto const layout = PipelineCache::self().pipeline_layout();
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, m_set, m_descriptor_set, {});
}
} // namespace spaced::graphics
