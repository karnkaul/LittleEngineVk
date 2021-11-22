#pragma once
#include <core/hash.hpp>
#include <graphics/render/pipeline_flags.hpp>
#include <graphics/render/vertex_input.hpp>
#include <graphics/shader.hpp>

namespace le::graphics {
struct FixedStateSpec {
	static constexpr std::size_t max_states_v = 8;
	static constexpr vk::DynamicState required_states_v[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

	ktl::fixed_vector<vk::DynamicState, max_states_v> dynamicStates = {std::begin(required_states_v), std::end(required_states_v)};
	vk::PolygonMode mode = vk::PolygonMode::eFill;
	vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
	PipelineFlags flags;
	f32 lineWidth = 1.0f;
};

struct PipelineSpec {
	struct Hasher;

	static std::size_t const default_hash_v;

	FixedStateSpec fixedState;
	VertexInputInfo vertexInput;
	Hash shaderURI;
	u32 subpass = 0;

	bool operator==(PipelineSpec const& rhs) const { return hash() == rhs.hash(); }
	Hash hash() const;
};

struct PipeData {
	vk::PipelineLayout layout;
	vk::Pipeline pipeline;
};

struct PipelineSpec::Hasher {
	std::size_t operator()(PipelineSpec const& spec) const;
};
} // namespace le::graphics
