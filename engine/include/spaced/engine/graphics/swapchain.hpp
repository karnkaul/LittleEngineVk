#pragma once
#include <glm/vec2.hpp>
#include <spaced/engine/graphics/image_view.hpp>
#include <deque>
#include <optional>
#include <span>
#include <vector>

namespace spaced::graphics {
struct Swapchain {
	struct Formats {
		std::vector<vk::SurfaceFormatKHR> srgb{};
		std::vector<vk::SurfaceFormatKHR> linear{};

		static auto make(std::span<vk::SurfaceFormatKHR const> available) -> Formats;
	};

	struct Storage {
		std::vector<ImageView> images{};
		std::vector<vk::UniqueImageView> views{};
		vk::UniqueSwapchainKHR swapchain{};
		glm::uvec2 extent{};
		std::optional<std::size_t> image_index{};
	};

	[[nodiscard]] auto make_create_info(vk::SurfaceKHR surface, std::uint32_t queue_family) const -> vk::SwapchainCreateInfoKHR;

	Formats formats{};
	std::vector<vk::PresentModeKHR> present_modes{};
	vk::SwapchainCreateInfoKHR create_info{};
	Storage active{};
	std::deque<Storage> retired{};
};
} // namespace spaced::graphics
