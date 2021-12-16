#pragma once
#include <graphics/image_ref.hpp>
#include <ktl/fixed_vector.hpp>

namespace le::graphics {
struct RenderTarget : ImageView {
	ImageRef ref() const noexcept { return {*this, false}; }
};

struct Framebuffer {
	RenderTarget colour;
	RenderTarget depth;
	ktl::fixed_vector<RenderTarget, 4> others;

	constexpr Extent2D extent() const noexcept { return colour.extent; }
};
} // namespace le::graphics
