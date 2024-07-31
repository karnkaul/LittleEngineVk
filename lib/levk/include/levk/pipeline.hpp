#pragma once
#include <levk/core/polymorphic.hpp>
#include <levk/pipeline_state.hpp>

namespace levk {
/// \brief Opaque interface for graphics pipelines.
class IPipeline : public Polymorphic {
  public:
	[[nodiscard]] virtual auto get_pipeline() const -> vk::Pipeline = 0;
	[[nodiscard]] virtual auto get_layout() const -> vk::PipelineLayout = 0;

	[[nodiscard]] virtual auto get_bindings(std::uint32_t set) const -> std::span<vk::DescriptorSetLayoutBinding const> = 0;

	[[nodiscard]] auto is_writable(std::uint32_t const set, std::uint32_t const binding) const -> bool {
		auto const bindings = get_bindings(set);
		if (binding >= bindings.size()) { return false; }
		return bindings[binding].descriptorCount > 0;
	}

	[[nodiscard]] virtual auto allocate_descriptor_set(std::uint32_t set_number) -> vk::DescriptorSet = 0;

	virtual void push_constants(vk::CommandBuffer command_buffer, void const* data, std::size_t size) const = 0;
	virtual void update_descriptor_sets(std::span<vk::WriteDescriptorSet const> writes) = 0;
	virtual void bind_descriptor_sets(vk::CommandBuffer command_buffer, std::span<vk::DescriptorSet const> descriptor_sets, std::uint32_t first_set) const = 0;

	void update_descriptor_set(vk::WriteDescriptorSet const write) { update_descriptor_sets({&write, 1}); }
	void bind_descriptor_set(vk::CommandBuffer const command_buffer, vk::DescriptorSet const descriptor_set, std::uint32_t const number) const {
		bind_descriptor_sets(command_buffer, {&descriptor_set, 1}, number);
	}
};
} // namespace levk
