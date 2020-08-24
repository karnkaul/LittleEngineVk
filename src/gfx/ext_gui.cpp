#include <stdexcept>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/colour.hpp>
#include <gfx/ext_gui.hpp>
#include <gfx/device.hpp>
#include <gfx/renderer_impl.hpp>
#include <window/window_impl.hpp>
#if defined(LEVK_USE_IMGUI)
#include <glm/gtc/color_space.hpp>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#if defined(LEVK_USE_GLFW)
#include <imgui_impl_glfw.h>
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
	g_pool = g_device.device.createDescriptorPool(pool_info);
	return g_pool;
}

void toSRGB(ImVec4& imColour)
{
	glm::vec3 colour = glm::convertSRGBToLinear(glm::vec3{imColour.x, imColour.y, imColour.z});
	imColour = {colour.x, colour.y, colour.z, imColour.w};
}

void setStyle()
{
	ImVec4* pColours = ImGui::GetStyle().Colors;

	toSRGB(pColours[ImGuiCol_Text]);
	toSRGB(pColours[ImGuiCol_TextDisabled]);
	toSRGB(pColours[ImGuiCol_WindowBg]);
	toSRGB(pColours[ImGuiCol_ChildBg]);
	toSRGB(pColours[ImGuiCol_PopupBg]);
	toSRGB(pColours[ImGuiCol_Border]);
	toSRGB(pColours[ImGuiCol_BorderShadow]);
	toSRGB(pColours[ImGuiCol_FrameBg]);
	toSRGB(pColours[ImGuiCol_FrameBgHovered]);
	toSRGB(pColours[ImGuiCol_FrameBgActive]);
	toSRGB(pColours[ImGuiCol_TitleBg]);
	toSRGB(pColours[ImGuiCol_TitleBgActive]);
	toSRGB(pColours[ImGuiCol_TitleBgCollapsed]);
	toSRGB(pColours[ImGuiCol_MenuBarBg]);
	toSRGB(pColours[ImGuiCol_ScrollbarBg]);
	toSRGB(pColours[ImGuiCol_ScrollbarGrab]);
	toSRGB(pColours[ImGuiCol_ScrollbarGrabHovered]);
	toSRGB(pColours[ImGuiCol_ScrollbarGrabActive]);
	toSRGB(pColours[ImGuiCol_CheckMark]);
	toSRGB(pColours[ImGuiCol_SliderGrab]);
	toSRGB(pColours[ImGuiCol_SliderGrabActive]);
	toSRGB(pColours[ImGuiCol_Button]);
	toSRGB(pColours[ImGuiCol_ButtonHovered]);
	toSRGB(pColours[ImGuiCol_ButtonActive]);
	toSRGB(pColours[ImGuiCol_Header]);
	toSRGB(pColours[ImGuiCol_HeaderHovered]);
	toSRGB(pColours[ImGuiCol_HeaderActive]);
	toSRGB(pColours[ImGuiCol_Separator]);
	toSRGB(pColours[ImGuiCol_SeparatorHovered]);
	toSRGB(pColours[ImGuiCol_SeparatorActive]);
	toSRGB(pColours[ImGuiCol_ResizeGrip]);
	toSRGB(pColours[ImGuiCol_ResizeGripHovered]);
	toSRGB(pColours[ImGuiCol_ResizeGripActive]);
	toSRGB(pColours[ImGuiCol_Tab]);
	toSRGB(pColours[ImGuiCol_TabHovered]);
	toSRGB(pColours[ImGuiCol_TabActive]);
	toSRGB(pColours[ImGuiCol_TabUnfocused]);
	toSRGB(pColours[ImGuiCol_TabUnfocusedActive]);
	toSRGB(pColours[ImGuiCol_PlotLines]);
	toSRGB(pColours[ImGuiCol_PlotLinesHovered]);
	toSRGB(pColours[ImGuiCol_PlotHistogram]);
	toSRGB(pColours[ImGuiCol_PlotHistogramHovered]);
	toSRGB(pColours[ImGuiCol_TextSelectedBg]);
	toSRGB(pColours[ImGuiCol_DragDropTarget]);
	toSRGB(pColours[ImGuiCol_NavHighlight]);
	toSRGB(pColours[ImGuiCol_NavWindowingHighlight]);
	toSRGB(pColours[ImGuiCol_NavWindowingDimBg]);
	toSRGB(pColours[ImGuiCol_ModalWindowDimBg]);
}
#endif
} // namespace

bool ext_gui::init([[maybe_unused]] Info const& info)
{
	bool bRet = false;
	if (!isInit())
	{
		ASSERT(info.window.payload != WindowID::s_null, "Invalid WindowID!");
#if defined(LEVK_USE_IMGUI)
		bRet = true;
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		setStyle();
#if defined(LEVK_USE_GLFW)
		auto pWindow = WindowImpl::nativeHandle(info.window).val<GLFWwindow*>();
		if (!pWindow)
		{
			bRet = false;
			LOG_E("Failed to get native window handle!");
		}
		else
		{
			ImGui_ImplGlfw_InitForVulkan(pWindow, true);
		}
#else
		LOG_E("NOT IMPLEMENTED");
		bRet = false;
#endif
		if (bRet)
		{
			ImGui_ImplVulkan_InitInfo initInfo = {};
			initInfo.Instance = g_instance.instance;
			initInfo.Device = g_device.device;
			initInfo.PhysicalDevice = g_instance.physicalDevice;
			initInfo.Queue = g_device.queues.graphics.queue;
			initInfo.QueueFamily = g_device.queues.graphics.familyIndex;
			initInfo.MinImageCount = (u32)info.minImageCount;
			initInfo.ImageCount = (u32)info.imageCount;
			initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			initInfo.DescriptorPool = createPool();
			bRet &= ImGui_ImplVulkan_Init(&initInfo, info.renderPass);
			if (bRet)
			{
				vk::CommandPoolCreateInfo poolInfo;
				poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
				poolInfo.queueFamilyIndex = g_device.queues.graphics.familyIndex;
				auto pool = g_device.device.createCommandPool(poolInfo);
				vk::CommandBufferAllocateInfo commandBufferInfo;
				commandBufferInfo.commandBufferCount = 1;
				commandBufferInfo.commandPool = pool;
				auto commandBuffer = g_device.device.allocateCommandBuffers(commandBufferInfo).front();
				vk::CommandBufferBeginInfo beginInfo;
				beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
				commandBuffer.begin(beginInfo);
				ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
				commandBuffer.end();
				vk::SubmitInfo endInfo;
				endInfo.commandBufferCount = 1;
				endInfo.pCommandBuffers = &commandBuffer;
				auto done = g_device.createFence(false);
				g_device.queues.graphics.queue.submit(endInfo, done);
				g_device.waitFor(done);
				ImGui_ImplVulkan_DestroyFontUploadObjects();
				g_device.destroy(pool, done);
			}
		}
#endif
		g_bInit = bRet;
	}
	return bRet;
}

void ext_gui::deinit()
{
	if (isInit())
	{
#if defined(LEVK_USE_IMGUI)
		ImGui_ImplVulkan_Shutdown();
		ImGui::DestroyContext();
		g_device.destroy(g_pool);
		g_pool = vk::DescriptorPool();
#endif
		g_bInit = false;
		g_bNewFrame = false;
	}
	return;
}

void ext_gui::newFrame()
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

void ext_gui::renderDrawData([[maybe_unused]] vk::CommandBuffer commandBuffer)
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

void ext_gui::render()
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

bool ext_gui::isInit()
{
	return g_bInit;
}
} // namespace le::gfx
