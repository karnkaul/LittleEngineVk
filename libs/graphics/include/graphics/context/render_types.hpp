#pragma once
#include <vulkan/vulkan.hpp>

namespace le::graphics {
struct RenderImage final {
	vk::Image image;
	vk::ImageView view;
	vk::Extent2D extent;
};

struct RenderTarget final {
	RenderImage colour;
	RenderImage depth;
	vk::Extent2D extent;

	inline std::array<vk::ImageView, 2> attachments() const { return {colour.view, depth.view}; }
};

struct RenderSync {
	vk::Semaphore drawReady;
	vk::Semaphore presentReady;
	vk::Fence drawing;
};
} // namespace le::graphics
