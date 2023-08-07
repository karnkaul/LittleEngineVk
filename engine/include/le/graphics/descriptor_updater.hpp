#pragma once
#include <le/graphics/cache/pipeline_cache.hpp>
#include <le/graphics/fallback.hpp>
#include <le/graphics/shader_layout.hpp>
#include <le/graphics/texture.hpp>

namespace le::graphics {
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

	[[nodiscard]] auto get_descriptor_set() const -> vk::DescriptorSet { return m_descriptor_set; }

	static auto bind_set(std::uint32_t set, vk::DescriptorSet descriptor_set, vk::CommandBuffer cmd) -> void;

  private:
	auto write(std::uint32_t binding, vk::DescriptorType type, void const* data, std::size_t size, vk::DeviceSize count) -> DescriptorUpdater&;

	vk::DescriptorSet m_descriptor_set{};
	std::uint32_t m_set{};
};
} // namespace le::graphics
