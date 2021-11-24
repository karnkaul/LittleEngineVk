#pragma once
#include <core/hash.hpp>
#include <core/span.hpp>
#include <graphics/render/pipeline_flags.hpp>
#include <graphics/render/vertex_input.hpp>

namespace le::graphics {
using SpirV = std::vector<u32>;

enum class ShaderType { eVertex, eFragment, eCompute, eCOUNT_ };
constexpr EnumArray<ShaderType, vk::ShaderStageFlagBits> g_shaderStages = {
	vk::ShaderStageFlagBits::eVertex,
	vk::ShaderStageFlagBits::eFragment,
	vk::ShaderStageFlagBits::eCompute,
};

struct ShaderSpec {
	template <typename T>
	using Map = EnumArray<ShaderType, T>;
	using ModMap = Map<vk::ShaderModule>;

	struct Module {
		ShaderType type = ShaderType::eVertex;
		Hash uri;
	};

	ktl::fixed_vector<Module, 4> modules;
};

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

	ShaderSpec shader;
	FixedStateSpec fixedState;
	VertexInputInfo vertexInput;
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
