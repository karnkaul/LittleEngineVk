#pragma once
#include <spaced/engine/core/not_null.hpp>
#include <spaced/engine/graphics/material.hpp>
#include <spaced/engine/graphics/pipeline_state.hpp>
#include <spaced/engine/graphics/primitive.hpp>
#include <spaced/engine/transform.hpp>
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
