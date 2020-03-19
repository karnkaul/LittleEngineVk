#pragma once
#include <functional>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "core/flags.hpp"
#include "engine/window/common.hpp"
#include "common.hpp"

namespace le::vuk
{
class Context final
{
public:
	enum class Flag
	{
		eRenderPaused = 0,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	struct UniformData
	{
		VkResource<vk::Buffer> buffer;
		vk::DescriptorSet descriptorSet;
		vk::DeviceSize offset;
	};

	struct FrameDriver final
	{
		struct UBOs
		{
			UniformData view;
		};

		UBOs ubos;
		vk::CommandBuffer commandBuffer;
	};

	using Write = std::function<void(FrameDriver::UBOs const&)>;
	using Draw = std::function<void(FrameDriver const&)>;

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

	struct Shared final
	{
		UBOData uboView;
	};

	struct SwapchainFrame final
	{
		vk::Framebuffer framebuffer;
		vk::ImageView colour;
		vk::ImageView depth;
		vk::Fence drawing;
		vk::CommandBuffer commandBuffer;
		vk::CommandPool commandPool;
		struct
		{
			UniformData view;
		} ubos;
	};

	struct Swapchain final
	{
		vk::Extent2D extent;
		std::vector<vk::Image> swapchainImages;
		std::vector<vk::ImageView> swapchainImageViews;
		VkResource<vk::Image> depthImage;
		vk::ImageView depthImageView;
		std::vector<SwapchainFrame> frames;
		vk::SwapchainKHR swapchain;
		vk::DescriptorPool descriptorPool;

		glm::ivec2 size = {};
		u32 currentImageIndex = 0;
		u8 imageCount = 0;

		SwapchainFrame& swapchainFrame();
	};

	struct RenderSync final
	{
		struct FrameSync final
		{
			vk::Semaphore render;
			vk::Semaphore present;
			vk::Fence drawing;
		};

		std::vector<FrameSync> frames;
		u32 index = 0;

		FrameSync& frameSync();
		void next();
	};

public:
	static std::string const s_tName;

private:
	static Shared const s_shared;

public:
	vk::RenderPass m_renderPass;
	u32 m_frameCount = 0;
	Flags m_flags;

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
	void onFramebufferResize();

	bool renderFrame(Write write, Draw draw, BeginPass const& pass = {});

private:
	void createRenderPass();
	bool createSwapchain();
	void destroySwapchain();
	void cleanup();

	bool recreateSwapchain();
};
} // namespace le::vuk
