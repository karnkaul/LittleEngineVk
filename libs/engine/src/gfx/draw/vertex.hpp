#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"

namespace le::gfx
{
struct Vertex final
{
	glm::vec2 position = {};
	glm::vec3 colour = {};

	static vk::VertexInputBindingDescription bindingDescription(u32 binding = 0);
	static std::vector<vk::VertexInputAttributeDescription> attributeDescriptions(u32 binding = 0);
};
} // namespace le::gfx
