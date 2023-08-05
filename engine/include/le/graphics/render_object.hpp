#pragma once
#include <le/core/not_null.hpp>
#include <le/core/transform.hpp>
#include <le/graphics/material.hpp>
#include <le/graphics/pipeline_state.hpp>
#include <le/graphics/primitive.hpp>
#include <span>

namespace le::graphics {
struct RenderInstance {
	Transform transform{};
	Rgba tint{white_v};
};

struct RenderObject {
	NotNull<Material const*> material;
	NotNull<Primitive const*> primitive;

	glm::mat4 parent{1.0f};
	std::span<RenderInstance const> instances{};
	std::span<glm::mat4 const> joints{};

	PipelineState pipeline_state{};
};
} // namespace le::graphics
