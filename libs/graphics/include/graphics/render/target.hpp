#pragma once
#include <graphics/common.hpp>

namespace le::graphics {
struct RenderImage {
	vk::Image image;
	vk::ImageView view;
	Extent2D extent;
};

struct RenderTarget {
	RenderImage colour;
	RenderImage depth;

	inline std::array<vk::ImageView, 2> attachments() const { return {colour.view, depth.view}; }
};
} // namespace le::graphics
