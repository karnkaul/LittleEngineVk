#include <iostream>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <core/ensure.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/instance.hpp>

[[maybe_unused]] static vk::DispatchLoaderDynamic g_dispatcher;

#if defined(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC) && VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
// Use default dispatcher
#define VK_DISPATCHER VULKAN_HPP_DEFAULT_DISPATCHER
// Allocate storage for default dispatcher
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;
#else
// Use custom dispatcher
#define VK_DISPATCHER g_dispatcher
#endif

namespace le::graphics {
// global for Device to temporarily disable (to suppress spam on Windows)
dl::level g_validationLevel = dl::level::warning;

namespace {
#define VK_LOG_MSG pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "UNKNOWN"

VKAPI_ATTR vk::Bool32 VKAPI_CALL validationCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
													VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void*) {
	static constexpr std::string_view name = "vk::validation";
	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		g_log.log(lvl::error, 0, "[{}] {}", name, VK_LOG_MSG);
		ENSURE(false, VK_LOG_MSG);
		return true;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		if (g_validationLevel <= dl::level::warning) {
			g_log.log(lvl::warning, 1, "[{}] {}", name, VK_LOG_MSG);
		}
		break;
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		if (g_validationLevel <= dl::level::info) {
			g_log.log(lvl::info, 1, "[{}] {}", name, VK_LOG_MSG);
		}
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		if (g_validationLevel <= dl::level::debug) {
			g_log.log(lvl::debug, 2, "[{}] {}", name, VK_LOG_MSG);
		}
		break;
	}
	return false;
}

bool findLayer(std::vector<vk::LayerProperties> const& available, char const* szLayer, std::optional<dl::level> log) {
	std::string_view const layerName(szLayer);
	for (auto& layer : available) {
		if (std::string_view(layer.layerName) == layerName) {
			return true;
		}
	}
	if (log) {
		dl::log(*log, "[{}] Requested layer [{}] not available!", g_name, szLayer);
	}
	return false;
}
} // namespace

Instance::Instance(Instance&& rhs)
	: m_metadata(std::move(rhs.m_metadata)), m_instance(std::exchange(rhs.m_instance, vk::Instance())),
	  m_loader(std::exchange(rhs.m_loader, vk::DispatchLoaderDynamic())), m_messenger(std::exchange(rhs.m_messenger, vk::DebugUtilsMessengerEXT())) {
}

Instance& Instance::operator=(Instance&& rhs) {
	if (&rhs != this) {
		destroy();
		m_metadata = std::move(rhs.m_metadata);
		m_instance = std::exchange(rhs.m_instance, vk::Instance());
		m_loader = std::exchange(rhs.m_loader, vk::DispatchLoaderDynamic());
		m_messenger = std::exchange(rhs.m_messenger, vk::DebugUtilsMessengerEXT());
	}
	return *this;
}

Instance::Instance(CreateInfo const& info) {
	static constexpr char const* szValidationLayer = "VK_LAYER_KHRONOS_validation";
	vk::DynamicLoader dl;
	VK_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	auto const layerProps = vk::enumerateInstanceLayerProperties();
	m_metadata.layers.clear();
	std::unordered_set<std::string_view> requiredExtensionsSet = {info.extensions.begin(), info.extensions.end()};
	bool bValidation = false;
	if (info.bValidation) {
		if (!findLayer(layerProps, szValidationLayer, dl::level::warning)) {
			ENSURE(false, "Validation layers requested but not present!");
		} else {
			requiredExtensionsSet.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			m_metadata.layers.push_back(szValidationLayer);
			bValidation = true;
		}
	}
	for (auto ext : requiredExtensionsSet) {
		m_metadata.extensions.push_back(ext.data());
	}
	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "LittleEngineVk Game";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "LittleEngineVk";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	vk::InstanceCreateInfo createInfo;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.ppEnabledExtensionNames = m_metadata.extensions.data();
	createInfo.enabledExtensionCount = (u32)m_metadata.extensions.size();
	createInfo.ppEnabledLayerNames = m_metadata.layers.data();
	createInfo.enabledLayerCount = (u32)m_metadata.layers.size();
	m_instance = vk::createInstance(createInfo, nullptr);
	VK_DISPATCHER.init(m_instance);
	m_loader = VK_DISPATCHER;
	if (bValidation) {
		vk::DebugUtilsMessengerCreateInfoEXT createInfo;
		using vksev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
		createInfo.messageSeverity = vksev::eError | vksev::eWarning | vksev::eInfo | vksev::eVerbose;
		using vktype = vk::DebugUtilsMessageTypeFlagBitsEXT;
		createInfo.messageType = vktype::eGeneral | vktype::ePerformance | vktype::eValidation;
		createInfo.pfnUserCallback = &validationCallback;
		ENSURE(m_loader.vkCreateDebugUtilsMessengerEXT, "Function pointer is null");
		m_messenger = m_instance.createDebugUtilsMessengerEXT(createInfo, nullptr, m_loader);
	}
	g_log.log(lvl::info, 1, "[{}] Vulkan instance constructed", g_name);
	g_validationLevel = info.validationLog;
}

Instance::~Instance() {
	destroy();
}

kt::fixed_vector<PhysicalDevice, 8> Instance::availableDevices(Span<std::string_view> required) const {
	kt::fixed_vector<PhysicalDevice, 8> ret;
	std::vector<vk::PhysicalDevice> const devices = m_instance.enumeratePhysicalDevices();
	for (auto const& device : devices) {
		std::unordered_set<std::string_view> missing(required.begin(), required.end());
		std::vector<vk::ExtensionProperties> const supported = device.enumerateDeviceExtensionProperties();
		for (std::size_t idx = 0; idx < supported.size() && !missing.empty(); ++idx) {
			missing.erase(std::string_view(supported[idx].extensionName));
		}
		if (missing.empty()) {
			PhysicalDevice available;
			available.properties = device.getProperties();
			available.queueFamilies = device.getQueueFamilyProperties();
			available.features = device.getFeatures();
			available.device = device;
			ret.push_back(std::move(available));
			if (ret.size() == ret.capacity()) {
				break;
			}
		}
	}
	return ret;
}

void Instance::destroy() {
	if (!Device::default_v(m_instance)) {
		if (!Device::default_v(m_messenger)) {
			m_instance.destroy(m_messenger, nullptr, m_loader);
			m_messenger = vk::DebugUtilsMessengerEXT();
		}
		g_log.log(lvl::info, 1, "[{}] Vulkan instance destroyed", g_name);
		m_instance.destroy();
		m_instance = vk::Instance();
	}
}
} // namespace le::graphics
