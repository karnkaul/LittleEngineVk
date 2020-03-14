#pragma once
#include <vulkan/vulkan.hpp>
#include "engine/window/common.hpp"
#include "common.hpp"

namespace le::vuk
{
class Context final
{
private:
	struct Info final
	{
		ContextData data;
		vk::SurfaceKHR surface;

		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;

		void refresh();
		bool isReady() const;
		vk::SurfaceFormatKHR bestColourFormat() const;
		vk::PresentModeKHR bestPresentMode() const;
		vk::Extent2D extent(glm::ivec2 const& framebufferSize) const;
	};

	struct Frame
	{
		vk::Framebuffer framebuffer;
		vk::ImageView colour;
		vk::ImageView depth;
		vk::Fence drawing;
	};

	struct Swapchain
	{
		vk::Extent2D extent;
		std::vector<vk::Image> swapchainImages;
		vk::DeviceMemory depthMemory;
		vk::Image depthImage;
		vk::ImageView depthImageView;
		std::vector<Frame> frames;
		vk::SwapchainKHR swapchain;
		vk::RenderPass renderPass;

		glm::ivec2 size = {};
		u8 imageCount = 0;
	};

public:
	static std::string const s_tName;

public:
	vk::Format m_colourFormat;
	vk::Format m_depthFormat;

private:
	Info m_info;
	Swapchain m_swapchain;
	WindowID m_window;
	u32 m_currentFrameIndex = 0;

public:
	Context(ContextData const& data);
	~Context();

	bool recreateSwapchain();

	Swapchain const& active() const;
	vk::RenderPassBeginInfo acquireNextImage(vk::Semaphore wait, vk::Fence setInUse);
	vk::Result present(vk::Semaphore wait);

private:
	bool createSwapchain();
	void destroySwapchain();
};
} // namespace le::vuk
