#pragma once
#include <core/not_null.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
class ShaderInput;

struct Pipeline {
	not_null<ShaderInput*> shaderInput;
	vk::Pipeline pipeline;
	vk::PipelineLayout layout;

	bool valid() const noexcept { return shaderInput && pipeline && layout; }
};
} // namespace le::graphics
