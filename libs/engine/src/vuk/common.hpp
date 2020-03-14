#pragma once
#include <functional>
#include <optional>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "core/flags.hpp"
#include "engine/window/common.hpp"

namespace le::vuk
{
using CreateSurface = std::function<vk::SurfaceKHR(vk::Instance)>;

struct AvailableDevice
{
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties properties;
	std::vector<vk::QueueFamilyProperties> queueFamilies;
};

struct InitData final
{
	enum class Flag
	{
		eValidation = 0,
		eTest,
		eCOUNT_
	};

	using Flags = TFlags<Flag>;
	using PickDevice = std::function<vk::PhysicalDevice(std::vector<AvailableDevice> const&)>;

	struct
	{
		std::vector<char const*> instanceExtensions;
		CreateSurface createTempSurface;
		u8 graphicsQueueCount = 1;
	} config;

	struct
	{
		PickDevice pickDevice;
		Flags flags;
	} options;
};

struct SwapchainData final
{
	struct
	{
		CreateSurface createNewSurface;
		glm::ivec2 framebufferSize = {};
		WindowID window;
	} config;

	struct Options
	{
		std::optional<vk::Format> format;
		std::optional<vk::ColorSpaceKHR> colourSpace;
	};
	Options options;
};
} // namespace le::vuk
