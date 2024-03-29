#pragma once
#include <le/core/nvec3.hpp>
#include <le/graphics/rgba.hpp>
#include <vector>

namespace le::graphics {
struct Lights {
	struct Directional {
		static constexpr float diffuse_intensity_v{5.0f};
		static constexpr float ambient_intensity_v{0.04f};

		nvec3 direction{-front_v};
		HdrRgba diffuse{white_v, diffuse_intensity_v};
		float ambient{ambient_intensity_v};
	};

	Directional primary{};
	std::vector<Directional> directional{};
};
} // namespace le::graphics
