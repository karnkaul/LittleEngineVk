#pragma once
#include <vulkan/vulkan.hpp>

namespace le::graphics {
struct ImageView {
	vk::Image image{};
	vk::ImageView view{};
	vk::Extent2D extent{};
	vk::Format format{};
};
} // namespace le::graphics
