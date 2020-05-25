#pragma once
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "core/log_config.hpp"
#include "core/std_types.hpp"
#include "core/flags.hpp"
#include "common.hpp"

namespace le::gfx
{
struct Info final
{
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceLimits deviceLimits;
	vk::Device device;

	f32 lineWidthMin = 1.0f;
	f32 lineWidthMax = 1.0f;
	log::Level validationLog = log::Level::eWarning;

	struct
	{
		Queue graphics;
		Queue present;
		Queue transfer;
	} queues;

	bool isValid(vk::SurfaceKHR surface) const;

	UniqueQueues uniqueQueues(QFlags flags) const;
	u32 findMemoryType(u32 typeFilter, vk::MemoryPropertyFlags properties) const;
	f32 lineWidth(f32 desired) const;
};

inline Info g_info;

struct Service final
{
	Service(InitInfo const& info);
	~Service();
};
} // namespace le::gfx
