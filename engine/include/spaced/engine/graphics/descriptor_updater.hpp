#pragma once
#include <spaced/engine/graphics/cache/pipeline_cache.hpp>
#include <spaced/engine/graphics/fallback.hpp>
#include <spaced/engine/graphics/shader_layout.hpp>
#include <spaced/engine/graphics/texture.hpp>

namespace spaced::graphics {
class DescriptorUpdater {
  public:
	explicit DescriptorUpdater(std::uint32_t set);

	[[nodiscard]] auto shader_layout() const -> ShaderLayout const&;

	auto update(std::uint32_t binding, vk::DescriptorType type, vk::DescriptorBufferInfo const& info, std::uint32_t count) -> DescriptorUpdater&;
	auto update(std::uint32_t binding, vk::DescriptorImageInfo const& info, std::uint32_t count) -> DescriptorUpdater&;

	auto write_uniform(std::uint32_t binding, void const* data, std::size_t size, vk::DeviceSize count = 1) -> DescriptorUpdater&;
	auto write_storage(std::uint32_t binding, void const* data, std::size_t size, vk::DeviceSize count = 1) -> DescriptorUpdater&;

	auto update_texture(std::uint32_t binding, Image const& texture, TextureSampler const& sampler) -> DescriptorUpdater&;
	auto update_texture(std::uint32_t binding, Texture const& texture) -> DescriptorUpdater&;

	auto bind_set(vk::CommandBuffer cmd) const -> void;

  private:
	auto write(std::uint32_t binding, vk::DescriptorType type, void const* data, std::size_t size, vk::DeviceSize count) -> DescriptorUpdater&;

	vk::DescriptorSet m_descriptor_set{};
	std::uint32_t m_set{};
};
} // namespace spaced::graphics
