#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
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

		vk::SurfaceFormatKHR colourFormat;
		vk::Format depthFormat = vk::Format::eD16Unorm;

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
		std::vector<vk::ImageView> swapchainImageViews;
		vuk::Image depthImage;
		vk::ImageView depthImageView;
		std::vector<Frame> frames;
		vk::SwapchainKHR swapchain;

		glm::ivec2 size = {};
		u32 currentImageIndex = 0;
		u8 imageCount = 0;
	};

	struct RenderSync final
	{
		struct FrameSync
		{
			struct
			{
				vk::Semaphore render;
				vk::Semaphore present;
				vk::Fence drawing;
			} sync;

			vk::CommandBuffer commandBuffer;
			vk::CommandPool pool;
			vk::RenderPassBeginInfo renderPassInfo;
		};

		std::vector<FrameSync> frames;
		u32 index = 0;

		FrameSync& frameSync();
		void next();
	};

public:
	static std::string const s_tName;

public:
	vk::RenderPass m_renderPass;
	u32 m_frameCount = 0;

private:
	Info m_info;
	Swapchain m_swapchain;
	RenderSync m_sync;
	WindowID m_window;

public:
	Context(ContextData const& data);
	~Context();

public:
	vk::Viewport transformViewport(ScreenRect const& nRect = {}, glm::vec2 const& depth = {0.0f, 1.0f}) const;
	vk::Rect2D transformScissor(ScreenRect const& nRect = {}) const;

public:
	vk::CommandBuffer beginRenderPass(BeginPass const& pass = {});
	void submitPresent();

private:
	void createRenderPass();
	void createSwapchain();
	void destroySwapchain();
	void cleanup();

	bool recreateSwapchain();
	vk::RenderPassBeginInfo acquireNextImage(vk::Semaphore wait, vk::Fence setInUse);
	vk::Result present(vk::Semaphore wait);
};
} // namespace le::vuk
