#pragma once
#include <optional>
#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"

namespace le::vuk
{
class Device final
{
public:
	struct QueueFamilyIndices final
	{
		std::optional<u32> graphics;
		std::optional<u32> presentation;

		constexpr bool isReady()
		{
			return graphics.has_value() && presentation.has_value();
		}
	};

public:
	static std::string const s_tName;

public:
	std::string m_name;
	QueueFamilyIndices m_queueFamilyIndices;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentationQueue;

private:
	vk::Device m_device;
	vk::SurfaceKHR m_surface;

public:
	Device(VkSurfaceKHR surface);
	~Device();
};
} // namespace le::vuk
