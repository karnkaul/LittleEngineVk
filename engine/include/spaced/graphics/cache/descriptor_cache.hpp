#pragma once
#include <spaced/core/mono_instance.hpp>
#include <spaced/graphics/buffering.hpp>
#include <vulkan/vulkan.hpp>

namespace spaced::graphics {
class DescriptorCache : public MonoInstance<DescriptorCache> {
  public:
	[[nodiscard]] auto allocate(vk::DescriptorSetLayout const& layout) -> vk::DescriptorSet;

	auto next_frame() -> void;
	auto clear() -> void;

  private:
	auto try_allocate(vk::DescriptorSetLayout const& layout, vk::DescriptorSet& out) const -> bool;

	struct Data {
		std::vector<vk::UniqueDescriptorPool> used{};
		std::vector<vk::UniqueDescriptorPool> free{};
		vk::UniqueDescriptorPool active{};
	};

	Buffered<Data> m_data{};
};
} // namespace spaced::graphics
