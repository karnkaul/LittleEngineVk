#pragma once
#include <levk/lights.hpp>

namespace levk {
struct RenderView {
	glm::mat4 camera_transform{1.0f};
	float camera_exposure{5.0f};
	DirectionalLight main_light{};
};
} // namespace levk
