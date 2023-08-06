#pragma once
#include <le/graphics/camera.hpp>
#include <le/graphics/lights.hpp>
#include <le/graphics/render_object.hpp>
#include <optional>

namespace le::graphics {
struct RenderFrame {
	NotNull<Lights const*> lights;
	NotNull<Camera const*> camera;
	std::vector<RenderObject> scene{};
	std::vector<RenderObject> ui{};
	std::optional<glm::vec2> world_frustum{};
	Rgba clear_colour{black_v};
};
} // namespace le::graphics
