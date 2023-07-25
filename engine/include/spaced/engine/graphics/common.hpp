#pragma once
#include <vulkan/vulkan.hpp>

namespace spaced::graphics {
struct ImageView {
	vk::Image image{};
	vk::ImageView view{};
	vk::Extent2D extent{};
	vk::Format format{};
};
} // namespace spaced::graphics
