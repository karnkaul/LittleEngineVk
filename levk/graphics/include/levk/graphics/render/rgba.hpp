#pragma once
#include <levk/core/colour.hpp>

namespace le::graphics {
struct RGBA {
	enum class Type { eIntensity, eAbsolute };

	Colour colour;
	Type type;

	constexpr RGBA(Colour colour = {}, Type type = Type::eIntensity) noexcept : colour(colour), type(type) {}

	glm::vec4 toVec4() const noexcept { return type == Type::eAbsolute ? colour.toRGB() : colour.toVec4(); }
};

struct DepthStencil {
	f32 depth{};
	u32 stencil{};
};
} // namespace le::graphics
