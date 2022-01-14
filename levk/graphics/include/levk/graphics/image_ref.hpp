#pragma once
#include <levk/graphics/common.hpp>

namespace le::graphics {
struct ImageView {
	vk::Image image;
	vk::ImageView view;
	Extent2D extent{};
	vk::Format format{};
};

struct ImageRef : ImageView {
	bool linear = false;
};
} // namespace le::graphics
