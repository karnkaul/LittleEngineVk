#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <glm/gtc/color_space.hpp>
#include <glm/mat4x4.hpp>
#include <spaced/engine/error.hpp>
#include <spaced/engine/graphics/command_buffer.hpp>
#include <spaced/engine/graphics/dear_imgui.hpp>
#include <spaced/engine/graphics/device.hpp>

namespace spaced::graphics {
DearImGui::DearImGui(Ptr<GLFWwindow> window, vk::Format colour, vk::Format depth) {
	auto& device = Device::self();

	static constexpr std::size_t multiplier_v{1000};

	auto const pool_sizes = std::array{
		vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, multiplier_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, multiplier_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, multiplier_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, multiplier_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eUniformTexelBuffer, multiplier_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eStorageTexelBuffer, multiplier_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, multiplier_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, multiplier_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, multiplier_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eStorageBufferDynamic, multiplier_v},
		vk::DescriptorPoolSize{vk::DescriptorType::eInputAttachment, multiplier_v},
	};
	auto dpci = vk::DescriptorPoolCreateInfo{};
	dpci.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	dpci.maxSets = multiplier_v * static_cast<std::uint32_t>(pool_sizes.size());
	dpci.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
	dpci.pPoolSizes = pool_sizes.data();
	m_pool = device.device().createDescriptorPoolUnique(dpci);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

	ImGui::StyleColorsDark();
	// NOLINTNEXTLINE
	for (int i = 0; i < ImGuiCol_COUNT; ++i) {
		// NOLINTNEXTLINE
		auto& colour = ImGui::GetStyle().Colors[i];
		auto const corrected = glm::convertSRGBToLinear(glm::vec4{colour.x, colour.y, colour.z, colour.w});
		colour = {corrected.x, corrected.y, corrected.z, corrected.w};
	}

	auto loader = vk::DynamicLoader{};
	auto get_fn = [&loader](char const* name) { return loader.getProcAddress<PFN_vkVoidFunction>(name); };
	auto lambda = +[](char const* name, void* ud) {
		// NOLINTNEXTLINE
		auto const* gf = reinterpret_cast<decltype(get_fn)*>(ud);
		return (*gf)(name);
	};
	ImGui_ImplVulkan_LoadFunctions(lambda, &get_fn);
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = device.instance();
	init_info.PhysicalDevice = device.physical_device();
	init_info.Device = device.device();
	init_info.QueueFamily = device.queue_family();
	init_info.Queue = device.queue();
	init_info.DescriptorPool = *m_pool;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(vk::SampleCountFlagBits::e1);

	ImGui_ImplVulkan_Init(&init_info, static_cast<VkFormat>(colour), static_cast<VkFormat>(depth));

	auto command_buffer = CommandBuffer{};
	ImGui_ImplVulkan_CreateFontsTexture(command_buffer.get());
	command_buffer.submit();
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	// overlay UI on existing colour image
	m_colour_load_op = vk::AttachmentLoadOp::eLoad;
}

DearImGui::~DearImGui() {
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

// NOLINTNEXTLINE
auto DearImGui::new_frame() -> void {
	if (m_state == State::eEndFrame) { end_frame(); }
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	m_state = State::eEndFrame;
}

// NOLINTNEXTLINE
auto DearImGui::end_frame() -> void {
	// ImGui::Render calls ImGui::EndFrame
	if (m_state == State::eNewFrame) { new_frame(); }
	ImGui::Render();
	m_state = State::eNewFrame;
}

auto DearImGui::render(vk::CommandBuffer const cmd) -> void {
	if (m_state == State::eEndFrame) { end_frame(); }
	if (auto* data = ImGui::GetDrawData()) { ImGui_ImplVulkan_RenderDrawData(data, cmd); }
}
} // namespace spaced::graphics
