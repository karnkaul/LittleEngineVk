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
};
} // namespace le::graphics
