#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "core/std_types.hpp"

namespace le::gfx
{
struct Vertex final
{
	constexpr static u32 binding = 0;

	glm::vec3 position = {};
	glm::vec3 colour = {};
	glm::vec2 texCoord = {};

	static vk::VertexInputBindingDescription bindingDescription();
	static std::vector<vk::VertexInputAttributeDescription> attributeDescriptions();
};
} // namespace le::gfx
