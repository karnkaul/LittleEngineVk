#pragma once
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <levk/core/is_positive.hpp>
#include <levk/core/radians.hpp>
#include <vulkan/vulkan.hpp>

namespace levk {
constexpr auto to_vk_extent(glm::uvec2 const vec) { return vk::Extent2D{vec.x, vec.y}; }
constexpr auto to_glm_vec2(vk::Extent2D const extent) { return glm::uvec2{extent.width, extent.height}; }

constexpr auto safe_image_extent(vk::Extent2D in) -> vk::Extent2D {
	if (in.width == 0) { in.width = 1; }
	if (in.height == 0) { in.height = 1; }
	return in;
}

constexpr auto safe_buffer_size(vk::DeviceSize in) -> vk::DeviceSize {
	if (in == 0) { in = 1; }
	return in;
}

constexpr auto scaled_extent(vk::Extent2D const in, float const scale) -> vk::Extent2D {
	glm::vec2 const ret = to_glm_vec2(in);
	return to_vk_extent(ret * scale);
}

[[nodiscard]] auto get_view_matrix(glm::vec3 const& position, Radians yaw, Radians pitch) -> glm::mat4;

inline void record_barriers(vk::CommandBuffer const command_buffer, std::span<vk::ImageMemoryBarrier2 const> image_barriers) {
	auto di = vk::DependencyInfo{};
	di.pImageMemoryBarriers = image_barriers.data();
	di.imageMemoryBarrierCount = static_cast<std::uint32_t>(image_barriers.size());
	command_buffer.pipelineBarrier2(di);
}
} // namespace levk
