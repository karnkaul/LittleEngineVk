#include <stdexcept>
#include "core/assert.hpp"
#include "core/colour.hpp"
#include "gui.hpp"
#include "info.hpp"
#include "utils.hpp"
#include "renderer_impl.hpp"
#include "window/window_impl.hpp"
#if defined(LEVK_USE_IMGUI)
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#if defined(LEVK_USE_GLFW)
#include "imgui_impl_glfw.h"
#endif
#endif

namespace le::gfx
{
namespace
{
bool g_bInit = false;
bool g_bNewFrame = false;

#if defined(LEVK_USE_IMGUI)
vk::DescriptorPool g_pool;

vk::DescriptorPool createPool()
{
	ASSERT(g_pool == vk::DescriptorPool(), "Duplicate pool!");
	vk::DescriptorPoolSize pool_sizes[] = {{vk::DescriptorType::eSampler, 1000},
										   {vk::DescriptorType::eCombinedImageSampler, 1000},
										   {vk::DescriptorType::eSampledImage, 1000},
										   {vk::DescriptorType::eStorageImage, 1000},
										   {vk::DescriptorType::eUniformTexelBuffer, 1000},
										   {vk::DescriptorType::eStorageTexelBuffer, 1000},
										   {vk::DescriptorType::eUniformBuffer, 1000},
										   {vk::DescriptorType::eStorageBuffer, 1000},
										   {vk::DescriptorType::eUniformBufferDynamic, 1000},
										   {vk::DescriptorType::eStorageBufferDynamic, 1000},
										   {vk::DescriptorType::eInputAttachment, 1000}};
	vk::DescriptorPoolCreateInfo pool_info;
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = (u32)(1000 * arraySize(pool_sizes));
	pool_info.poolSizeCount = (u32)arraySize(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	g_pool = g_info.device.createDescriptorPool(pool_info);
	return g_pool;
}
#endif
} // namespace

bool gui::init(Info const& info)
{
	bool bRet = false;
	if (!isInit())
	{
		ASSERT(info.window != WindowID(), "Invalid WindowID!");
#if defined(LEVK_USE_IMGUI)
		bRet = true;
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
#if defined(LEVK_USE_GLFW)
		auto pWindow = reinterpret_cast<::GLFWwindow*>(WindowImpl::nativeHandle(info.window));
		ImGui_ImplGlfw_InitForVulkan(pWindow, true);
#else
		LOG_E("NOT IMPLEMENTED");
		bRet = false;
#endif
		if (bRet)
		{
			ImGui_ImplVulkan_InitInfo initInfo = {};
			initInfo.Instance = g_info.instance;
			initInfo.Device = g_info.device;
			initInfo.PhysicalDevice = g_info.physicalDevice;
			initInfo.Queue = g_info.queues.graphics;
			initInfo.QueueFamily = g_info.queueFamilyIndices.graphics;
			initInfo.MinImageCount = (u32)info.minImageCount;
			initInfo.ImageCount = (u32)info.imageCount;
			initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			initInfo.DescriptorPool = createPool();
			bRet &= ImGui_ImplVulkan_Init(&initInfo, info.renderPass);
			if (bRet)
			{
				vk::CommandPoolCreateInfo poolInfo;
				poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
				poolInfo.queueFamilyIndex = g_info.queueFamilyIndices.graphics;
				auto pool = g_info.device.createCommandPool(poolInfo);
				vk::CommandBufferAllocateInfo commandBufferInfo;
				commandBufferInfo.commandBufferCount = 1;
				commandBufferInfo.commandPool = pool;
				auto commandBuffer = g_info.device.allocateCommandBuffers(commandBufferInfo).front();
				vk::CommandBufferBeginInfo beginInfo;
				beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
				commandBuffer.begin(beginInfo);
				ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
				commandBuffer.end();
				vk::SubmitInfo endInfo;
				endInfo.commandBufferCount = 1;
				endInfo.pCommandBuffers = &commandBuffer;
				auto done = createFence(false);
				g_info.queues.graphics.submit(endInfo, done);
				waitFor(done);
				ImGui_ImplVulkan_DestroyFontUploadObjects();
				vkDestroy(pool, done);
			}
		}
#endif
		g_bInit = bRet;
	}
	return bRet;
}

void gui::deinit()
{
	if (isInit())
	{
#if defined(LEVK_USE_IMGUI)
		ImGui_ImplVulkan_Shutdown();
		ImGui::DestroyContext();
		vkDestroy(g_pool);
		g_pool = vk::DescriptorPool();
#endif
		g_bInit = false;
		g_bNewFrame = false;
	}
	return;
}

void gui::newFrame()
{
	if (isInit() && !g_bNewFrame)
	{
#if defined(LEVK_USE_IMGUI)
		ImGui_ImplVulkan_NewFrame();
#if defined(LEVK_USE_GLFW)
		ImGui_ImplGlfw_NewFrame();
#else
		LOG_E("NOT IMPLEMENTED");
		return;
#endif
		ImGui::NewFrame();
#endif
		g_bNewFrame = true;
	}
	return;
}

void gui::renderDrawData([[maybe_unused]] vk::CommandBuffer commandBuffer)
{
	if (isInit())
	{
#if defined(LEVK_USE_IMGUI)
		if (auto pData = ImGui::GetDrawData())
		{
			ImGui_ImplVulkan_RenderDrawData(pData, commandBuffer);
		}
#endif
	}
	return;
}

void gui::render()
{
	if (isInit() && g_bNewFrame)
	{
#if defined(LEVK_USE_IMGUI)
		ImGui::Render();
#endif
		g_bNewFrame = false;
	}
	return;
}

bool gui::isInit()
{
	return g_bInit;
}
} // namespace le::gfx
