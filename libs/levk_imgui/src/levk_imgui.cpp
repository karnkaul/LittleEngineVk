#include <stdexcept>
#include <levk_imgui/levk_imgui.hpp>

#if defined(LEVK_USE_IMGUI)
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <core/colour.hpp>
#include <core/ensure.hpp>
#include <core/log.hpp>
#include <glm/common.hpp>
#include <glm/gtc/color_space.hpp>
#include <graphics/context/device.hpp>
#include <window/desktop_instance.hpp>
#endif

namespace le {
using namespace graphics;
using namespace window;

#if defined(LEVK_USE_IMGUI)
namespace {
vk::DescriptorPool createPool(Device& device, u32 count) {
	vk::DescriptorPoolSize pool_sizes[] = {{vk::DescriptorType::eSampler, count},
										   {vk::DescriptorType::eCombinedImageSampler, count},
										   {vk::DescriptorType::eSampledImage, count},
										   {vk::DescriptorType::eStorageImage, count},
										   {vk::DescriptorType::eUniformTexelBuffer, count},
										   {vk::DescriptorType::eStorageTexelBuffer, count},
										   {vk::DescriptorType::eUniformBuffer, count},
										   {vk::DescriptorType::eStorageBuffer, count},
										   {vk::DescriptorType::eUniformBufferDynamic, count},
										   {vk::DescriptorType::eStorageBufferDynamic, count},
										   {vk::DescriptorType::eInputAttachment, count}};
	vk::DescriptorPoolCreateInfo pool_info;
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = (u32)(count * arraySize(pool_sizes));
	pool_info.poolSizeCount = (u32)arraySize(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	return device.device().createDescriptorPool(pool_info);
}

void toSRGB(ImVec4& imColour) {
	glm::vec3 const colour = glm::convertSRGBToLinear(glm::vec3{imColour.x, imColour.y, imColour.z});
	imColour = {colour.x, colour.y, colour.z, imColour.w};
}

void setStyle() {
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
} // namespace
#endif

DearImGui::DearImGui(Device& device) : TMonoInstance(false), m_device(device) {
}

DearImGui::DearImGui(Device& device, [[maybe_unused]] DesktopInstance const& window, [[maybe_unused]] CreateInfo const& info)
	: TMonoInstance(true), m_device(device) {
#if defined(LEVK_USE_IMGUI)
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	if (info.texFormat == Texture::srgbFormat) {
		setStyle();
	}
	auto const glfwWindow = window.nativePtr();
	ENSURE(glfwWindow.contains<GLFWwindow*>(), "Invalid Window!");
	ImGui_ImplGlfw_InitForVulkan(glfwWindow.get<GLFWwindow*>(), true);
	ImGui_ImplVulkan_InitInfo initInfo = {};
	auto const& queue = device.queues().queue(QType::eGraphics);
	m_pool = createPool(device, info.descriptorCount);
	initInfo.Instance = device.m_instance.get().instance();
	initInfo.Device = device.device();
	initInfo.PhysicalDevice = device.physicalDevice().device;
	initInfo.Queue = queue.queue;
	initInfo.QueueFamily = queue.familyIndex;
	initInfo.MinImageCount = (u32)info.minImageCount;
	initInfo.ImageCount = (u32)info.imageCount;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.DescriptorPool = m_pool;
	if (!ImGui_ImplVulkan_Init(&initInfo, info.renderPass)) {
		device.destroy(m_pool);
		throw std::runtime_error("ImGui_ImplVulkan_Init failed");
	}
	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = queue.familyIndex;
	auto pool = device.device().createCommandPool(poolInfo);
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandBufferCount = 1;
	commandBufferInfo.commandPool = pool;
	auto commandBuffer = device.device().allocateCommandBuffers(commandBufferInfo).front();
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer.begin(beginInfo);
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	commandBuffer.end();
	vk::SubmitInfo endInfo;
	endInfo.commandBufferCount = 1;
	endInfo.pCommandBuffers = &commandBuffer;
	auto done = device.createFence(false);
	queue.queue.submit(endInfo, done);
	device.waitFor(done);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
	device.destroy(pool, done);
	logD("[DearImGui] constructed");
#endif
}

DearImGui::~DearImGui() {
#if defined(LEVK_USE_IMGUI)
	if (m_bActive) {
		ImGui_ImplVulkan_Shutdown();
		ImGui::DestroyContext();
		m_device.get().destroy(m_pool);
		logD("[DearImGui] destroyed");
	}
#endif
}

bool DearImGui::beginFrame() {
#if defined(LEVK_USE_IMGUI)
	if (m_bActive && next(State::eEnd, State::eBegin)) {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		return true;
	}
#endif
	return false;
}

bool DearImGui::render() {
#if defined(LEVK_USE_IMGUI)
	if (m_bActive && next(State::eRender, State::eRender)) {
		ImGui::Render();
		return true;
	}
#endif
	return false;
}

bool DearImGui::endFrame([[maybe_unused]] vk::CommandBuffer commandBuffer) {
#if defined(LEVK_USE_IMGUI)
	if (m_bActive && next(State::eRender, State::eEnd)) {
		if (auto const pData = ImGui::GetDrawData()) {
			ImGui_ImplVulkan_RenderDrawData(pData, commandBuffer);
			return true;
		}
	}
#endif
	return false;
}

bool DearImGui::demo([[maybe_unused]] bool* show) const {
#if defined(LEVK_USE_IMGUI)
	if (m_bActive && m_state == State::eBegin) {
		ImGui::ShowDemoWindow(show);
		return true;
	}
#endif
	return false;
}

bool DearImGui::next([[maybe_unused]] State from, [[maybe_unused]] State to) {
#if defined(LEVK_USE_IMGUI)
	if (m_state == from) {
		m_state = to;
		return true;
	}
#endif
	return false;
}
} // namespace le
