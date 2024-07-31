#pragma once
#include <vulkan/vulkan.hpp>
#include <variant>

namespace levk {
struct DescriptorInfo {
	using Payload = std::variant<std::monostate, vk::DescriptorBufferInfo, vk::DescriptorImageInfo>;

	[[nodiscard]] auto has_value() const -> bool { return !std::holds_alternative<std::monostate>(payload); }
	[[nodiscard]] auto get_write_descriptor_set(vk::DescriptorSet descriptor_set) const -> vk::WriteDescriptorSet;

	Payload payload{std::monostate{}};
	vk::DescriptorType type{};
	std::uint32_t binding{};
};
} // namespace levk
