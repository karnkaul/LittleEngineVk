#pragma once
#include <levk/primitive.hpp>
#include <levk/render_instance.hpp>

namespace levk {
struct RenderObject {
	PrimitivesView primitives{};
	std::span<RenderInstance const> instances{};
	glm::mat4 parent{1.0f};
	std::span<glm::mat4 const> joint_matrices{};
	std::optional<vk::PolygonMode> polygon_mode{};
	float line_width{1.0f};
	bool disable_depth_test{false};
	std::optional<vk::CompareOp> depth_compare{};
	bool alpha_blend{true};
};
} // namespace levk
