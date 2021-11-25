#include <levk_imgui/levk_imgui.hpp>
#include <stdexcept>

#if defined(LEVK_USE_IMGUI)
#include <GLFW/glfw3.h>
#if defined(LEVK_USE_GLFW)
#include <backends/imgui_impl_glfw.h>
#endif
#include <backends/imgui_impl_vulkan.h>
#include <core/colour.hpp>
#include <core/log.hpp>
#include <core/utils/error.hpp>
#include <glm/common.hpp>
#include <graphics/render/command_buffer.hpp>
#include <graphics/render/context.hpp>
#include <window/glue.hpp>
#endif

namespace le {
using namespace graphics;
using namespace window;

#define MU [[maybe_unused]]

#if defined(LEVK_USE_IMGUI)
namespace {
using DIS = DearImGui::State;

vk::DescriptorPool makePool(Device& device, u32 count) {
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

void correct(ImVec4& imColour) {
	glm::vec4 const colour = Colour({imColour.x, imColour.y, imColour.z, imColour.w}).toRGB();
	imColour = {colour.x, colour.y, colour.z, colour.w};
}

void fixStyle() {
	ImVec4* pColours = ImGui::GetStyle().Colors;
	correct(pColours[ImGuiCol_Text]);
	correct(pColours[ImGuiCol_TextDisabled]);
	correct(pColours[ImGuiCol_WindowBg]);
	correct(pColours[ImGuiCol_ChildBg]);
	correct(pColours[ImGuiCol_PopupBg]);
	correct(pColours[ImGuiCol_Border]);
	correct(pColours[ImGuiCol_BorderShadow]);
	correct(pColours[ImGuiCol_FrameBg]);
	correct(pColours[ImGuiCol_FrameBgHovered]);
	correct(pColours[ImGuiCol_FrameBgActive]);
	correct(pColours[ImGuiCol_TitleBg]);
	correct(pColours[ImGuiCol_TitleBgActive]);
	correct(pColours[ImGuiCol_TitleBgCollapsed]);
	correct(pColours[ImGuiCol_MenuBarBg]);
	correct(pColours[ImGuiCol_ScrollbarBg]);
	correct(pColours[ImGuiCol_ScrollbarGrab]);
	correct(pColours[ImGuiCol_ScrollbarGrabHovered]);
	correct(pColours[ImGuiCol_ScrollbarGrabActive]);
	correct(pColours[ImGuiCol_CheckMark]);
	correct(pColours[ImGuiCol_SliderGrab]);
	correct(pColours[ImGuiCol_SliderGrabActive]);
	correct(pColours[ImGuiCol_Button]);
	correct(pColours[ImGuiCol_ButtonHovered]);
	correct(pColours[ImGuiCol_ButtonActive]);
	correct(pColours[ImGuiCol_Header]);
	correct(pColours[ImGuiCol_HeaderHovered]);
	correct(pColours[ImGuiCol_HeaderActive]);
	correct(pColours[ImGuiCol_Separator]);
	correct(pColours[ImGuiCol_SeparatorHovered]);
	correct(pColours[ImGuiCol_SeparatorActive]);
	correct(pColours[ImGuiCol_ResizeGrip]);
	correct(pColours[ImGuiCol_ResizeGripHovered]);
	correct(pColours[ImGuiCol_ResizeGripActive]);
	correct(pColours[ImGuiCol_Tab]);
	correct(pColours[ImGuiCol_TabHovered]);
	correct(pColours[ImGuiCol_TabActive]);
	correct(pColours[ImGuiCol_TabUnfocused]);
	correct(pColours[ImGuiCol_TabUnfocusedActive]);
	correct(pColours[ImGuiCol_PlotLines]);
	correct(pColours[ImGuiCol_PlotLinesHovered]);
	correct(pColours[ImGuiCol_PlotHistogram]);
	correct(pColours[ImGuiCol_PlotHistogramHovered]);
	correct(pColours[ImGuiCol_TextSelectedBg]);
	correct(pColours[ImGuiCol_DragDropTarget]);
	correct(pColours[ImGuiCol_NavHighlight]);
	correct(pColours[ImGuiCol_NavWindowingHighlight]);
	correct(pColours[ImGuiCol_NavWindowingDimBg]);
	correct(pColours[ImGuiCol_ModalWindowDimBg]);
}
} // namespace
#endif

DearImGui::DearImGui() = default;

DearImGui::DearImGui(MU not_null<RenderContext*> context, MU not_null<Window const*> window, MU std::size_t descriptorCount) {
#if defined(LEVK_USE_IMGUI) && defined(LEVK_USE_GLFW)
	m_device = context->vram().m_device;
	static vk::Instance s_inst;
	static vk::DynamicLoader const s_dl;
	s_inst = m_device->instance();
	auto const loader = [](char const* fn, void*) { return s_dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr")(s_inst, fn); };
	ImGui_ImplVulkan_LoadFunctions(loader);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	if (context->colourCorrection() == graphics::ColourCorrection::eAuto) { fixStyle(); }
	ImGui::GetStyle().WindowRounding = 0.0f;
	ImGui_ImplGlfw_InitForVulkan(glfwPtr(*window), true);
	ImGui_ImplVulkan_InitInfo initInfo = {};
	auto const& queue = m_device->queues().queue(QType::eGraphics);
	m_pool = {m_device, makePool(*m_device, (u32)descriptorCount)};
	initInfo.Instance = m_device->instance();
	initInfo.Device = m_device->device();
	initInfo.PhysicalDevice = m_device->physicalDevice().device;
	initInfo.Queue = queue.queue;
	initInfo.QueueFamily = queue.familyIndex;
	initInfo.MinImageCount = context->surface().minImageCount();
	initInfo.ImageCount = context->surface().imageCount();
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.DescriptorPool = static_cast<VkDescriptorPool>(m_pool.get());
	if (!ImGui_ImplVulkan_Init(&initInfo, context->renderer().renderPass())) { throw std::runtime_error("ImGui_ImplVulkan_Init failed"); }
	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = queue.familyIndex;
	auto pool = m_device->device().createCommandPool(poolInfo);
	vk::CommandBufferAllocateInfo commandBufferInfo;
	commandBufferInfo.commandBufferCount = 1;
	commandBufferInfo.commandPool = pool;
	auto commandBuffer = m_device->device().allocateCommandBuffers(commandBufferInfo).front();
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer.begin(beginInfo);
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	commandBuffer.end();
	vk::SubmitInfo endInfo;
	endInfo.commandBufferCount = 1;
	endInfo.pCommandBuffers = &commandBuffer;
	auto done = m_device->makeFence(false);
	queue.queue.submit(endInfo, done);
	m_device->waitFor(done);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
	m_device->destroy(pool, done);
	m_del = {m_device, nullptr};
	logD("[DearImGui] constructed");
#endif
}

void DearImGui::Del::operator()(vk::Device, void*) const {
#if defined(LEVK_USE_IMGUI)
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	logD("[DearImGui] destroyed");
#endif
}

bool DearImGui::beginFrame() {
#if defined(LEVK_USE_IMGUI)
	if (m_del.active()) {
		if (m_state == State::eBegin) { ImGui::Render(); }
		next(m_state, State::eBegin);
		ImGui_ImplVulkan_NewFrame();
#if defined(LEVK_USE_GLFW)
		ImGui_ImplGlfw_NewFrame();
#endif
		ImGui::NewFrame();
		if (m_showDemo) { ImGui::ShowDemoWindow(&m_showDemo); }
		return true;
	}
#endif
	return false;
}

bool DearImGui::endFrame() {
#if defined(LEVK_USE_IMGUI)
	if (m_del.active() && next(State::eBegin, State::eRender)) {
		ImGui::Render();
		return true;
	}
#endif
	return false;
}

bool DearImGui::renderDrawData([[maybe_unused]] graphics::CommandBuffer const& cb) {
#if defined(LEVK_USE_IMGUI)
	if (m_del.active() && next(State::eRender, State::eEnd)) {
		if (auto const pData = ImGui::GetDrawData()) {
			ImGui_ImplVulkan_RenderDrawData(pData, cb.m_cb);
			return true;
		}
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
