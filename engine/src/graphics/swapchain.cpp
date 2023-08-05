#include <le/error.hpp>
#include <le/graphics/swapchain.hpp>

namespace le::graphics {
namespace {
constexpr auto optimal_present_mode(std::span<vk::PresentModeKHR const> modes) -> vk::PresentModeKHR {
	auto ret = vk::PresentModeKHR::eFifoRelaxed;
	if (std::ranges::find(modes, ret) != modes.end()) { return ret; }
	ret = vk::PresentModeKHR::eMailbox;
	if (std::ranges::find(modes, ret) != modes.end()) { return ret; }
	return vk::PresentModeKHR::eFifo;
}
} // namespace

auto Swapchain::Formats::make(std::span<vk::SurfaceFormatKHR const> available) -> Formats {
	auto ret = Formats{};
	for (auto const format : available) {
		if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			if (is_srgb_format(format.format)) {
				ret.srgb.push_back(format);
			} else {
				ret.linear.push_back(format);
			}
		}
	}
	return ret;
}

auto Swapchain::make_create_info(vk::SurfaceKHR surface, std::uint32_t queue_family) const -> vk::SwapchainCreateInfoKHR {
	auto ret = vk::SwapchainCreateInfoKHR{};
	auto const format = formats.srgb.front();
	ret.surface = surface;
	ret.presentMode = optimal_present_mode(present_modes);
	ret.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
	ret.queueFamilyIndexCount = 1u;
	ret.pQueueFamilyIndices = &queue_family;
	ret.imageColorSpace = format.colorSpace;
	ret.imageArrayLayers = 1u;
	ret.imageFormat = format.format;
	return ret;
}
} // namespace le::graphics
