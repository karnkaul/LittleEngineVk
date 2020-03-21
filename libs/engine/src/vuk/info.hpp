#pragma once
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"
#include "core/flags.hpp"
#include "vuk/common.hpp"

namespace le::vuk
{
struct Info final
{
	enum class QFlag
	{
		ePresent = 0,
		eTransfer,
		eGraphics,
		eCOUNT_
	};
	using QFlags = TFlags<QFlag>;

	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceLimits deviceLimits;
	vk::Device device;

	f32 lineWidthMin = 1.0f;
	f32 lineWidthMax = 1.0f;

	struct
	{
		u32 graphics;
		u32 present;
		u32 transfer;
	} queueFamilyIndices;

	struct
	{
		vk::Queue graphics;
		vk::Queue present;
		vk::Queue transfer;
	} queues;

	bool isValid(vk::SurfaceKHR surface) const;
	std::vector<u32> uniqueQueueIndices(QFlags flags) const;
	vk::SharingMode sharingMode(QFlags flags) const;
	u32 findMemoryType(u32 typeFilter, vk::MemoryPropertyFlags properties) const;
	f32 lineWidth(f32 desired) const;
};

inline Info g_info;

struct Service final
{
	Service(InitData const& initData);
	~Service();
};
} // namespace le::vuk
