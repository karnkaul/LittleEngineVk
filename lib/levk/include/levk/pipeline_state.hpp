#pragma once
#include <levk/render_shader.hpp>
#include <levk/vertex.hpp>
#include <vulkan/vulkan.hpp>

namespace levk {
/// \brief Primitive pipeline state.
struct RenderPrimitiveState {
	vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
	VertexBinding vertex_binding{VertexBinding::eDefault};
	bool alpha_blend{true};

	auto operator==(RenderPrimitiveState const&) const -> bool = default;
};

/// \brief Camera pipeline state.
struct RenderPassState {
	vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
	vk::Format colour_format{vk::Format::eR8G8B8A8Srgb};
	std::optional<vk::Format> depth_format{};
	vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
	vk::CompareOp depth_compare{vk::CompareOp::eLess};
	bool depth_test{true};

	auto operator==(RenderPassState const&) const -> bool = default;
};

/// \brief Fixed state for a pipeline.
struct PipelineState {
	RenderPrimitiveState primitive_state{};
	RenderPassState pass_state{};

	auto operator==(PipelineState const&) const -> bool = default;
};
} // namespace levk
