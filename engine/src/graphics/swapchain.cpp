#include <spaced/error.hpp>
#include <spaced/graphics/swapchain.hpp>

namespace spaced::graphics {
namespace {
constexpr vk::Format srgb_formats_v[] = {vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb, vk::Format::eA8B8G8R8SrgbPack32};
} // namespace

auto Swapchain::Formats::make(std::span<vk::SurfaceFormatKHR const> available) -> Formats {
	auto ret = Formats{};
	for (auto const format : available) {
		if (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			if (std::ranges::find(srgb_formats_v, format.format) != std::ranges::end(srgb_formats_v)) {
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
	ret.presentMode = vk::PresentModeKHR::eFifo;
	ret.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
	ret.queueFamilyIndexCount = 1u;
	ret.pQueueFamilyIndices = &queue_family;
	ret.imageColorSpace = format.colorSpace;
	ret.imageArrayLayers = 1u;
	ret.imageFormat = format.format;
	return ret;
}
} // namespace spaced::graphics
