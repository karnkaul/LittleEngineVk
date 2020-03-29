#pragma once
#include <glm/glm.hpp>
#include "core/std_types.hpp"
#include "gfx/vulkan.hpp"

namespace le::gfx
{
struct Vertex final
{
	constexpr static u32 binding = 0;

	glm::vec2 position = {};
	glm::vec3 colour = {};

	static vk::VertexInputBindingDescription bindingDescription();
	static std::vector<vk::VertexInputAttributeDescription> attributeDescriptions();
};
} // namespace le::gfx
