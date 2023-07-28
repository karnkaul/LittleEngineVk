#pragma once
#include <cstdint>

namespace spaced::graphics {
struct TextureSampler {
	enum class Wrap : std::uint8_t { eRepeat, eClampEdge, eClampBorder };
	enum class Filter : std::uint8_t { eLinear, eNearest };
	enum class Border : std::uint8_t { eOpaqueBlack, eOpaqueWhite, eTransparentBlack };

	Wrap wrap_s{Wrap::eRepeat};
	Wrap wrap_t{Wrap::eRepeat};
	Filter min{Filter::eLinear};
	Filter mag{Filter::eLinear};
	Border border{Border::eOpaqueBlack};

	auto operator==(TextureSampler const&) const -> bool = default;
};
} // namespace spaced::graphics
