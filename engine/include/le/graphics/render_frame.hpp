#pragma once
#include <le/graphics/camera.hpp>
#include <le/graphics/lights.hpp>
#include <le/graphics/render_object.hpp>
#include <span>

namespace le::graphics {
struct RenderFrame {
	NotNull<Lights const*> lights;
	NotNull<Camera const*> camera;
	std::span<RenderObject const> scene{};
	std::span<RenderObject const> ui{};
};
} // namespace le::graphics
