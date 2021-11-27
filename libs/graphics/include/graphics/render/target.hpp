#pragma once
#include <graphics/common.hpp>

namespace le::graphics {
struct RenderTarget {
	vk::Image image;
	vk::ImageView view;
	Extent2D extent{};
	vk::Format format{};
};

struct Framebuffer {
	RenderTarget colour;
	RenderTarget depth;
	std::vector<RenderTarget> images;

	constexpr Extent2D extent() const noexcept { return colour.extent; }
};
} // namespace le::graphics
