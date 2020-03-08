#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "engine/window/window_id.hpp"
#include "swapchain_data.hpp"

namespace le::vuk
{
class Swapchain final
{
public:
	struct Details final
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
	vk::Format m_format;
	vk::Extent2D m_extent;

private:
	Details m_details;
	vk::SwapchainKHR m_swapchain;
	std::vector<vk::Image> m_images;
	std::vector<vk::ImageView> m_imageViews;
	std::vector<vk::Framebuffer> m_framebuffers;
	vk::RenderPass m_defaultRenderPass;
	WindowID m_window;

public:
	Swapchain(SwapchainData const& data, WindowID window);
	~Swapchain();

	void recreate(SwapchainData const& data);
	void destroy();

public:
	operator vk::SwapchainKHR const&() const;

private:
	void getDetails(vk::SurfaceKHR const& surface);
	void createSwapchain(SwapchainData const& data);
	void populateImages();
	void createRenderPasses();
	void createFramebuffers();
};
} // namespace le::vuk
