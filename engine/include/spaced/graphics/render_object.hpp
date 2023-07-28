#pragma once
#include <spaced/core/not_null.hpp>
#include <spaced/core/transform.hpp>
#include <spaced/graphics/material.hpp>
#include <spaced/graphics/pipeline_state.hpp>
#include <spaced/graphics/primitive.hpp>
#include <span>

namespace spaced::graphics {
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
} // namespace spaced::graphics
