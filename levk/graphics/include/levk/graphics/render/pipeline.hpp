#pragma once
#include <levk/core/not_null.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
class ShaderInput;

struct Pipeline {
	not_null<ShaderInput*> input;
	vk::Pipeline pipeline;
	vk::PipelineLayout layout;

	constexpr explicit operator bool() const { return pipeline && layout; }
	constexpr bool operator==(Pipeline const&) const = default;
};
} // namespace le::graphics
