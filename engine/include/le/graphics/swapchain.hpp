#pragma once
#include <glm/vec2.hpp>
#include <le/graphics/image_view.hpp>
#include <algorithm>
#include <deque>
#include <optional>
#include <span>
#include <vector>

namespace le::graphics {
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

	static constexpr auto srgb_formats_v = std::array{vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb, vk::Format::eA8B8G8R8SrgbPack32};

	static constexpr auto is_srgb_format(vk::Format const format) { return std::ranges::find(srgb_formats_v, format) != srgb_formats_v.end(); }

	[[nodiscard]] auto make_create_info(vk::SurfaceKHR surface, std::uint32_t queue_family) const -> vk::SwapchainCreateInfoKHR;

	Formats formats{};
	std::vector<vk::PresentModeKHR> present_modes{};
	vk::SwapchainCreateInfoKHR create_info{};
	Storage active{};
	std::deque<Storage> retired{};
};
} // namespace le::graphics
