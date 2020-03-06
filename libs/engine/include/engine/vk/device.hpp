#pragma once
#include <optional>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "core/std_types.hpp"

namespace le::vuk
{
class Device final
{
public:
	struct Data final
	{
		vk::SurfaceKHR surface;
		glm::ivec2 windowSize;
	};

	struct QueueFamilyIndices final
	{
		std::optional<u32> graphics;
		std::optional<u32> present;

		constexpr bool isReady()
		{
			return graphics.has_value() && present.has_value();
		}
	};

	struct SwapchainDetails final
	{
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;

		bool isReady() const;
		vk::SurfaceFormatKHR bestFormat() const;
		vk::PresentModeKHR bestPresentMode() const;
		vk::Extent2D extent(glm::ivec2 const& windowSize) const;
	};

public:
	static std::string const s_tName;

public:
	std::string m_name;
	QueueFamilyIndices m_queueFamilyIndices;
	SwapchainDetails m_swapchainDetails;
	vk::Format m_swapchainFormat;
	vk::Extent2D m_swapchainExtent;

private:
	vk::Device m_device;
	vk::SurfaceKHR m_surface;
	vk::SwapchainKHR m_swapchain;
	std::vector<vk::Image> m_swapchainImages;
	std::vector<vk::ImageView> m_swapchainImageViews;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentationQueue;

public:
	Device(Data&& data);
	~Device();

private:
	bool createSwapchain(vk::PhysicalDevice const& physicalDevice, glm::ivec2 const& windowSize);
};
} // namespace le::vuk
