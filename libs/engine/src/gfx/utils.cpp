#include <algorithm>
#include <set>
#include "info.hpp"
#include "utils.hpp"

namespace le
{
TResult<vk::Format> gfx::supportedFormat(PriorityList<vk::Format> const& desired, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
	for (auto format : desired)
	{
		vk::FormatProperties props = g_info.physicalDevice.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	return {};
}

void gfx::waitFor(vk::Fence optional)
{
	if (optional != vk::Fence())
	{
		g_info.device.waitForFences(optional, true, maxVal<u64>());
	}
}

void gfx::waitAll(vk::ArrayProxy<const vk::Fence> validFences)
{
	if (!validFences.empty())
	{
		g_info.device.waitForFences(std::move(validFences), true, maxVal<u64>());
	}
}

bool gfx::isSignalled(vk::Fence fence)
{
	if (fence != vk::Fence())
	{
		return g_info.device.getFenceStatus(fence) == vk::Result::eSuccess;
	}
	return true;
}

bool gfx::allSignalled(ArrayView<vk::Fence const> fences)
{
	return std::all_of(fences.begin(), fences.end(), [](auto fence) { return isSignalled(fence); });
}

vk::ImageView gfx::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, vk::ImageViewType type)
{
	vk::ImageViewCreateInfo createInfo;
	createInfo.image = image;
	createInfo.viewType = type;
	createInfo.format = format;
	createInfo.components.r = createInfo.components.g = createInfo.components.b = createInfo.components.a = vk::ComponentSwizzle::eIdentity;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;
	return g_info.device.createImageView(createInfo);
}
} // namespace le
