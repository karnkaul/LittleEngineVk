#include <algorithm>
#include <memory>
#include <set>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "window/window_impl.hpp"
#include "info.hpp"
#include "vram.hpp"
#include "draw/resources.hpp"

namespace le::gfx
{
namespace
{
static std::string const s_tInstance = utils::tName<vk::Instance>();
static std::string const s_tDevice = utils::tName<vk::Device>();

vk::DispatchLoaderDynamic g_loader;
vk::DebugUtilsMessengerEXT g_debugMessenger;

#define VK_LOG_MSG pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "UNKNOWN"

VKAPI_ATTR vk::Bool32 VKAPI_CALL validationCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
													VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void*)
{
	static std::string_view const name = "vk::validation";
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		LOG_E("[{}] {}", name, VK_LOG_MSG);
		return true;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		LOG_W("[{}] {}", name, VK_LOG_MSG);
		break;
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		LOG_I("[{}] {}", name, VK_LOG_MSG);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		LOG_D("[{}] {}", name, VK_LOG_MSG);
		break;
	}
	return false;
}

vk::Device initDevice(vk::Instance instance, std::vector<char const*> const& layers, InitData const& initData)
{
	ASSERT(initData.config.graphicsQueueCount > 0, "Invalid queue count!");
	vk::Device device;
	vk::SurfaceKHR surface;
	std::string deviceName;
	std::vector<char const*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	try
	{
		surface = initData.config.createTempSurface(instance);
		auto physicalDevices = vk::Instance(instance).enumeratePhysicalDevices();
		std::vector<AvailableDevice> availableDevices;
		availableDevices.reserve(physicalDevices.size());
		for (auto const& physicalDevice : physicalDevices)
		{
			std::set<std::string_view> missingExtensions(deviceExtensions.begin(), deviceExtensions.end());
			auto const extensions = physicalDevice.enumerateDeviceExtensionProperties();
			for (size_t idx = 0; idx < extensions.size() && !missingExtensions.empty(); ++idx)
			{
				missingExtensions.erase(std::string_view(extensions.at(idx).extensionName));
			}
			for (auto extension : missingExtensions)
			{
				LOG_E("[{}] Required extensions not present on physical device [{}]!", s_tDevice, extension);
			}
			ASSERT(missingExtensions.empty(), "Required extension not present!");
			if (missingExtensions.empty())
			{
				AvailableDevice availableDevice;
				availableDevice.properties = physicalDevice.getProperties();
				availableDevice.queueFamilies = physicalDevice.getQueueFamilyProperties();
				availableDevice.physicalDevice = physicalDevice;
				if (availableDevice.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
				{
					g_info.physicalDevice = physicalDevice;
				}
				availableDevices.push_back(std::move(availableDevice));
			}
		}
		if (initData.options.pickDevice)
		{
			g_info.physicalDevice = initData.options.pickDevice(availableDevices);
		}
		if (g_info.physicalDevice == vk::PhysicalDevice())
		{
			instance.destroy(surface);
			throw std::runtime_error("Failed to select a physical device!");
		}
		auto const properties = g_info.physicalDevice.getProperties();
		deviceName = properties.deviceName;
		g_info.deviceLimits = properties.limits;
		g_info.lineWidthMin = properties.limits.lineWidthRange[0];
		g_info.lineWidthMax = properties.limits.lineWidthRange[1];
		auto const queueFamilies = g_info.physicalDevice.getQueueFamilyProperties();
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		std::optional<u32> graphicsFamily, presentFamily, transferFamily;
		for (size_t idx = 0; idx < queueFamilies.size(); ++idx)
		{
			auto const& queueFamily = queueFamilies.at(idx);
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				graphicsFamily.emplace((u32)idx);
			}
			if (graphicsFamily.has_value() && (u32)idx != graphicsFamily.value() && queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
			{
				transferFamily.emplace((u32)idx);
			}
			if (g_info.physicalDevice.getSurfaceSupportKHR((u32)idx, surface))
			{
				presentFamily.emplace((u32)idx);
			}
		}
		if (!graphicsFamily.has_value() || !presentFamily.has_value())
		{
			throw std::runtime_error("Failed to obtain graphics/present queues from device!");
		}
		std::set<u32> uniqueFamilies = {(u32)graphicsFamily.value(), (u32)presentFamily.value()};
		if (transferFamily.has_value())
		{
			uniqueFamilies.emplace(transferFamily.value());
		}
		f32 priority = 1.0f;
		for (auto family : uniqueFamilies)
		{
			vk::DeviceQueueCreateInfo queueCreateInfo;
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfo.queueCount = family == graphicsFamily.value() ? initData.config.graphicsQueueCount : 1;
			queueCreateInfo.pQueuePriorities = &priority;
			queueCreateInfos.push_back(std::move(queueCreateInfo));
		}
		vk::PhysicalDeviceFeatures deviceFeatures;
		deviceFeatures.fillModeNonSolid = true;
		deviceFeatures.wideLines = g_info.lineWidthMax > 1.0f;
		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		if (!layers.empty())
		{
			deviceCreateInfo.enabledLayerCount = (u32)layers.size();
			deviceCreateInfo.ppEnabledLayerNames = layers.data();
		}
		deviceCreateInfo.enabledExtensionCount = (u32)deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data();
		device = g_info.physicalDevice.createDevice(deviceCreateInfo);
		g_info.queueFamilyIndices.graphics = graphicsFamily.value();
		g_info.queueFamilyIndices.present = presentFamily.value();
		g_info.queueFamilyIndices.transfer = transferFamily.has_value() ? transferFamily.value() : graphicsFamily.value();
	}
	catch (std::exception const& e)
	{
		if (device != vk::Device())
		{
			device.destroy();
		}
		instance.destroy(surface);
		if (g_debugMessenger != vk::DebugUtilsMessengerEXT())
		{
			instance.destroy(g_debugMessenger, nullptr, g_loader);
		}
		instance.destroy();
		throw std::runtime_error(e.what());
	}
	instance.destroy(surface);
	LOG_I("[{}] constructed. Using GPU: [{}]", s_tDevice, deviceName);
	return device;
}

void init(InitData const& initData)
{
	std::vector<char const*> requiredLayers;
	std::set<char const*> requiredExtensionsSet = {initData.config.instanceExtensions.begin(), initData.config.instanceExtensions.end()};
	InitData::Flags flags;
	flags.set(InitData::Flag::eValidation, InitData::Flag::eTest);
	if (initData.options.flags.isSet(InitData::Flag::eValidation))
	{
		requiredExtensionsSet.emplace(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
	}
	std::vector<char const*> const requiredExtensions = {requiredExtensionsSet.begin(), requiredExtensionsSet.end()};
	auto const layers = vk::enumerateInstanceLayerProperties();
	for (auto szRequiredLayer : requiredLayers)
	{
		bool bFound = false;
		std::string_view const requiredLayer(szRequiredLayer);
		for (auto const& layer : layers)
		{
			if (requiredLayer == std::string_view(layer.layerName))
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			LOG_E("[{}] Required layer [{}] not available!", s_tInstance, szRequiredLayer);
			throw std::runtime_error("Failed to create Instance!");
		}
	}
	vk::Instance instance;
	{
		vk::ApplicationInfo appInfo;
		appInfo.pApplicationName = "LittleEngineVk Game";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "LittleEngineVk";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		vk::InstanceCreateInfo createInfo;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.ppEnabledExtensionNames = requiredExtensions.empty() ? nullptr : requiredExtensions.data();
		createInfo.enabledExtensionCount = (u32)requiredExtensions.size();
		createInfo.ppEnabledLayerNames = requiredLayers.empty() ? nullptr : requiredLayers.data();
		createInfo.enabledLayerCount = (u32)requiredLayers.size();
		instance = vk::createInstance(createInfo, nullptr);
	}
	vk::DynamicLoader dl;
	g_loader.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	g_loader.init(instance);
	if (initData.options.flags.isSet(InitData::Flag::eValidation))
	{
		vk::DebugUtilsMessengerCreateInfoEXT createInfo;
		createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
									 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
		createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
								 | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
		createInfo.pfnUserCallback = &validationCallback;
		try
		{
			g_debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo, nullptr, g_loader);
		}
		catch (std::exception const& e)
		{
			instance.destroy();
			throw std::runtime_error(e.what());
		}
	}
	auto device = initDevice(instance, requiredLayers, initData);
	vram::init(instance, device, g_info.physicalDevice);

	g_info.instance = instance;
	g_info.device = device;
	g_info.queues.graphics = device.getQueue(g_info.queueFamilyIndices.graphics, 0);
	g_info.queues.present = device.getQueue(g_info.queueFamilyIndices.present, 0);
	g_info.queues.transfer = device.getQueue(g_info.queueFamilyIndices.transfer, 0);
	LOG_I("[{}] and [{}] successfully initialised", s_tInstance, s_tDevice);
}

void deinit()
{
	resources::unloadAll();
	vram::deinit();
	if (g_info.device != vk::Device())
	{
		g_info.device.destroy();
	}
	if (g_info.instance != vk::Instance())
	{
		if (g_debugMessenger != vk::DebugUtilsMessengerEXT())
		{
			g_info.instance.destroy(g_debugMessenger, nullptr, g_loader);
		}
		g_info.instance.destroy();
	}
	g_info = Info();
	return;
}
} // namespace

bool Info::isValid(vk::SurfaceKHR surface) const
{
	return physicalDevice != vk::PhysicalDevice() ? physicalDevice.getSurfaceSupportKHR(queueFamilyIndices.present, surface) : false;
}

std::vector<u32> Info::uniqueQueueIndices(QFlags flags) const
{
	std::vector<u32> indices;
	indices.reserve(3);
	if (flags.isSet(QFlag::eGraphics))
	{
		indices.push_back(queueFamilyIndices.graphics);
	}
	if (flags.isSet(QFlag::ePresent) && queueFamilyIndices.graphics != queueFamilyIndices.present)
	{
		indices.push_back(queueFamilyIndices.present);
	}
	if (flags.isSet(QFlag::eTransfer) && queueFamilyIndices.transfer != queueFamilyIndices.graphics)
	{
		indices.push_back(queueFamilyIndices.transfer);
	}
	return indices;
}

vk::SharingMode Info::sharingMode(QFlags flags) const
{
	if (flags.isSet(QFlag::ePresent) && !flags.isSet(QFlag::eTransfer))
	{
		return queueFamilyIndices.graphics == queueFamilyIndices.present ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
	}
	if (!flags.isSet(QFlag::ePresent) && flags.isSet(QFlag::eTransfer))
	{
		return queueFamilyIndices.transfer != queueFamilyIndices.graphics ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
	}
	if (queueFamilyIndices.graphics == queueFamilyIndices.present && queueFamilyIndices.transfer != queueFamilyIndices.graphics)
	{
		return vk::SharingMode::eExclusive;
	}
	return vk::SharingMode::eConcurrent;
}

u32 Info::findMemoryType(u32 typeFilter, vk::MemoryPropertyFlags properties) const
{
	auto const memProperties = physicalDevice.getMemoryProperties();
	for (u32 i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("Failed to find suitable memory type!");
}

f32 Info::lineWidth(f32 desired) const
{
	return std::clamp(desired, lineWidthMin, lineWidthMax);
}

Service::Service(InitData const& initData)
{
	init(initData);
}

Service::~Service()
{
	deinit();
}
} // namespace le::gfx
