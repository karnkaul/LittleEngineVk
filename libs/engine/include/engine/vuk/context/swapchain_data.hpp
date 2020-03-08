#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace le::vuk
{
struct SwapchainData final
{
	vk::SurfaceKHR surface;
	glm::ivec2 framebufferSize;
};
} // namespace le::vuk
