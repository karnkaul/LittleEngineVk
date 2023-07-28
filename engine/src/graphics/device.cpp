#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <spaced/core/logger.hpp>
#include <spaced/graphics/device.hpp>
#include <spaced/graphics/resource.hpp>

namespace spaced::graphics {
namespace {
auto make_instance(std::vector<char const*> extensions, RenderDeviceCreateInfo const& gdci, Device::Info& out) -> vk::UniqueInstance {
	static constexpr std::string_view validation_layer_v = "VK_LAYER_KHRONOS_validation";
	auto vdl = vk::DynamicLoader{};
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vdl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	if (gdci.validation) {
		auto const available_layers = vk::enumerateInstanceLayerProperties();
		auto layer_search = [](vk::LayerProperties const& properties) { return properties.layerName == validation_layer_v; };
		if (std::ranges::find_if(available_layers, layer_search) != available_layers.end()) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			out.validation = true;
		} else {
			out.validation = false;
		}
	}

	// TODO: build version
	// auto const version = VK_MAKE_VERSION(version_v.major, version_v.minor, version_v.patch);
	auto const version = VK_MAKE_VERSION(0, 1, 0);
	auto const vai = vk::ApplicationInfo{"spaced", version, "spaced", version, VK_API_VERSION_1_3};
	auto ici = vk::InstanceCreateInfo{};
	ici.pApplicationInfo = &vai;
	out.portability = false;
#if defined(__APPLE__)
	ici.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
	out.portability = true;
#endif
	ici.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
	ici.ppEnabledExtensionNames = extensions.data();
	if (out.validation) {
		static constexpr char const* layers = validation_layer_v.data();
		ici.enabledLayerCount = 1;
		ici.ppEnabledLayerNames = &layers;
	}
	auto ret = vk::UniqueInstance{};
	try {
		ret = vk::createInstanceUnique(ici);
	} catch (vk::LayerNotPresentError const& e) {
		logger::Logger{"vk"}.error("{}", e.what());
		ici.enabledLayerCount = 0;
		ret = vk::createInstanceUnique(ici);
	}
	if (!ret) { throw Error{"Failed to createe Vulkan Instance"}; }
	VULKAN_HPP_DEFAULT_DISPATCHER.init(ret.get());
	return ret;
}

auto make_debug_messenger(vk::Instance instance) -> vk::UniqueDebugUtilsMessengerEXT {
	auto validationCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
								 VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void*) -> vk::Bool32 {
		static auto const log{logger::Logger{"vk"}};
		std::string_view const msg = (pCallbackData != nullptr) && (pCallbackData->pMessage != nullptr) ? pCallbackData->pMessage : "UNKNOWN";
		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: log.error("{}", msg); return vk::Bool32{1};
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: log.warn("{}", msg); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: log.info("{}", msg); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: log.debug("{}", msg); break;
		default: break;
		}
		return vk::Bool32{0};
	};

	auto dumci = vk::DebugUtilsMessengerCreateInfoEXT{};
	using vksev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
	dumci.messageSeverity = vksev::eError | vksev::eWarning | vksev::eInfo | vksev::eVerbose;
	using vktype = vk::DebugUtilsMessageTypeFlagBitsEXT;
	dumci.messageType = vktype::eGeneral | vktype::ePerformance | vktype::eValidation;
	dumci.pfnUserCallback = validationCallback;
	try {
		return instance.createDebugUtilsMessengerEXTUnique(dumci, nullptr);
	} catch (std::exception const& e) { throw Error{e.what()}; }
}

auto get_queue_family(vk::PhysicalDevice const& device, vk::SurfaceKHR const surface) -> std::optional<std::uint32_t> {
	static constexpr auto queue_flags_v = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;
	auto const properties = device.getQueueFamilyProperties();
	for (std::size_t i = 0; i < properties.size(); ++i) {
		auto const family = static_cast<std::uint32_t>(i);
		if (device.getSurfaceSupportKHR(family, surface) == 0) { continue; }
		if (!(properties[i].queueFlags & queue_flags_v)) { continue; }
		return family;
	}
	return {};
}

struct Gpu {
	vk::PhysicalDevice device{};
	vk::PhysicalDeviceProperties properties{};
	std::uint32_t queue_family{};
};

auto select_gpu(vk::Instance const instance, vk::SurfaceKHR const surface) -> Gpu {
	enum class Preference {
		eDiscrete = 10,
	};
	struct Entry {
		Gpu gpu{};
		int preference{};
		auto operator<=>(Entry const& rhs) const { return preference <=> rhs.preference; }
	};
	auto const get_queue_family = [surface](vk::PhysicalDevice const& device, std::uint32_t& out_family) {
		static constexpr auto queue_flags_v = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;
		auto const properties = device.getQueueFamilyProperties();
		for (std::size_t i = 0; i < properties.size(); ++i) {
			auto const family = static_cast<std::uint32_t>(i);
			if (device.getSurfaceSupportKHR(family, surface) == 0) { continue; }
			if (!(properties[i].queueFlags & queue_flags_v)) { continue; }
			out_family = family;
			return true;
		}
		return false;
	};
	auto const devices = instance.enumeratePhysicalDevices();
	auto entries = std::vector<Entry>{};
	for (auto const& device : devices) {
		auto entry = Entry{.gpu = {device}};
		entry.gpu.properties = device.getProperties();
		if (!get_queue_family(device, entry.gpu.queue_family)) { continue; }
		if (entry.gpu.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) { entry.preference += static_cast<int>(Preference::eDiscrete); }
		entries.push_back(entry);
	}
	if (entries.empty()) { throw Error{"Failed to find a suitable Vulkan Physical Device"}; }
	std::sort(entries.begin(), entries.end());
	return entries.back().gpu;
}

auto make_device(Gpu const& gpu) -> vk::UniqueDevice {
	static constexpr float priority_v = 1.0f;
	static constexpr std::array required_extensions_v = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,

#if defined(__APPLE__)
		"VK_KHR_portability_subset",
#endif
	};

	auto qci = vk::DeviceQueueCreateInfo{{}, gpu.queue_family, 1, &priority_v};
	auto dci = vk::DeviceCreateInfo{};
	auto enabled = vk::PhysicalDeviceFeatures{};
	auto available_features = gpu.device.getFeatures();
	enabled.fillModeNonSolid = available_features.fillModeNonSolid;
	enabled.wideLines = available_features.wideLines;
	enabled.samplerAnisotropy = available_features.samplerAnisotropy;
	enabled.sampleRateShading = available_features.sampleRateShading;
	auto const available_extensions = gpu.device.enumerateDeviceExtensionProperties();
	for (auto const* ext : required_extensions_v) {
		auto const found = [ext](vk::ExtensionProperties const& props) { return std::string_view{props.extensionName} == ext; };
		if (std::ranges::find_if(available_extensions, found) == available_extensions.end()) {
			throw Error{std::format("Required extension '{}' not supported by selected GPU '{}'", ext, gpu.properties.deviceName.data())};
		}
	}

	auto dynamic_rendering_feature = vk::PhysicalDeviceDynamicRenderingFeatures{1};
	auto synchronization_2_feature = vk::PhysicalDeviceSynchronization2FeaturesKHR{1};
	synchronization_2_feature.pNext = &dynamic_rendering_feature;

	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
	dci.enabledExtensionCount = static_cast<std::uint32_t>(required_extensions_v.size());
	dci.ppEnabledExtensionNames = required_extensions_v.data();
	dci.pEnabledFeatures = &enabled;
	dci.pNext = &synchronization_2_feature;

	auto ret = gpu.device.createDeviceUnique(dci);
	if (!ret) { throw Error{"Failed to create Vulkan Device"}; }

	VULKAN_HPP_DEFAULT_DISPATCHER.init(ret.get());
	return ret;
}
} // namespace

Device::Device(Ptr<GLFWwindow> window, CreateInfo const& create_info) {
	auto count = std::uint32_t{};
	auto const* result = glfwGetRequiredInstanceExtensions(&count);
	// NOLINTNEXTLINE
	auto extensions = std::vector<char const*>(result, result + static_cast<std::size_t>(count));
	m_instance = make_instance(std::move(extensions), create_info, m_info);

	if (m_info.validation) { m_debug_messenger = make_debug_messenger(*m_instance); }

	// NOLINTNEXTLINE
	auto glfw_surface = VkSurfaceKHR{};
	if (glfwCreateWindowSurface(instance(), window, {}, &glfw_surface) != VK_SUCCESS) { throw Error{"Failed to create Vulkan Surface"}; }
	m_surface = vk::UniqueSurfaceKHR{glfw_surface, instance()};

	auto const gpu = select_gpu(instance(), surface());
	m_physical_device = gpu.device;
	m_device_properties = gpu.properties;
	m_queue_family = gpu.queue_family;

	m_device = make_device(gpu);
	m_queue = m_device->getQueue(queue_family(), 0);

	m_allocator = std::make_unique<graphics::Allocator>(instance(), physical_device(), device());
}

Device::~Device() { m_device->waitIdle(); }

auto Device::wait_for(vk::Fence const fence, std::uint64_t const timeout) const -> bool {
	return device().waitForFences(fence, vk::Bool32{1}, timeout) == vk::Result::eSuccess;
}

auto Device::reset(vk::Fence const fence, bool wait_first) const -> bool {
	if (wait_first && !wait_for(fence)) { return false; }
	device().resetFences(fence);
	return true;
}

auto Device::acquire_next_image(vk::SwapchainKHR swapchain, std::uint32_t& out_image_index, vk::Semaphore signal) const -> vk::Result {
	auto lock = std::scoped_lock{m_mutex};
	return device().acquireNextImageKHR(swapchain, Device::max_timeout_v, signal, {}, &out_image_index);
}

auto Device::submit(vk::SubmitInfo2 const& submit_info, vk::Fence const signal) const -> bool {
	auto lock = std::scoped_lock{m_mutex};
	return queue().submit2(1, &submit_info, signal) == vk::Result::eSuccess;
}

auto Device::submit_and_present(vk::SubmitInfo2 const& submit_info, vk::Fence const submit_signal, vk::PresentInfoKHR const& present_info) const -> vk::Result {
	auto lock = std::scoped_lock{m_mutex};
	[[maybe_unused]] auto const result = queue().submit2(1, &submit_info, submit_signal);
	return queue().presentKHR(&present_info);
}
} // namespace spaced::graphics
