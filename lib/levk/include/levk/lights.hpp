#pragma once
#include <levk/colour.hpp>
#include <levk/transform.hpp>

namespace levk {
/// \brief Base light.
struct Light {
	RgbU8 tint{colour::white_v};
	float intensity{2.0f};

	void inspect_light();
};

/// \brief Directional light.
struct DirectionalLight : Light {
	glm::quat orientation{get_look_forward()};

	void inspect_directional_light();
};
} // namespace levk
