#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "engine/window/common.hpp"
#include "vuk/common.hpp"

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
		vk::SurfaceFormatKHR bestFormat(SwapchainData::Options options) const;
		vk::PresentModeKHR bestPresentMode() const;
		vk::Extent2D extent(glm::ivec2 const& framebufferSize) const;
	};

public:
	static std::string const s_tName;

public:
	vk::RenderPass m_defaultRenderPass;

private:
	Details m_details;
	vk::SwapchainKHR m_swapchain;
	vk::SurfaceKHR m_surface;
	vk::Format m_format;
	vk::Extent2D m_extent;

	std::vector<vk::Image> m_images;
	std::vector<vk::ImageView> m_imageViews;
	std::vector<vk::Framebuffer> m_framebuffers;
	std::vector<vk::Fence> m_inUseFences;

	CreateSurface m_getNewSurface;
	SwapchainData::Options m_options;
	glm::ivec2 m_framebufferSize = {};

	u32 m_currentIndex = 0;
	WindowID m_window;

public:
	Swapchain(SwapchainData const& data);
	~Swapchain();

	void recreate(glm::ivec2 const& framebufferSize);

public:
	vk::Format format() const;
	vk::RenderPassBeginInfo acquireNextImage(vk::Semaphore wait, vk::Fence setInUse);
	vk::Result present(vk::Semaphore wait) const;
	operator vk::SwapchainKHR() const;

private:
	void create();
	void getDetails();
	void createSwapchain();
	void populateImages();
	void createRenderPasses();
	void createFramebuffers();
	void release();
};
} // namespace le::vuk
