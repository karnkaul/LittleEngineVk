#pragma once
#include <core/std_types.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
struct VertexInputInfo {
	static constexpr std::size_t max_v = 32;

	ktl::fixed_vector<vk::VertexInputBindingDescription, max_v> bindings;
	ktl::fixed_vector<vk::VertexInputAttributeDescription, max_v> attributes;
};

template <typename T>
struct VertexInfoFactory {
	struct VertexInputInfo operator()(u32 binding) const;
};
} // namespace le::graphics
