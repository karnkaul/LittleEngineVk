#include <algorithm>
#include <cstring>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/vk/instance.hpp"

namespace le
{
namespace
{
VKAPI_ATTR VkBool32 VKAPI_CALL validationCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
												  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		LOG_E("[{}] {}", utils::tName<VkInstance>(), pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		LOG_W("[{}] {}", utils::tName<VkInstance>(), pCallbackData->pMessage);
		break;
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		LOG_I("[{}] {}", utils::tName<VkInstance>(), pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		LOG_D("[{}] {}", utils::tName<VkInstance>(), pCallbackData->pMessage);
		break;
	}
	return VK_FALSE;
}
} // namespace

VkInstance::~VkInstance()
{
	destroy();
}

bool VkInstance::init(Data data)
{
	if (m_instance != vk::Instance())
	{
		LOG_W("[{}] already initialised!", utils::tName<VkInstance>());
		return true;
	}
	if (data.bAddValidationLayers)
	{
		data.extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		data.layers.push_back("VK_LAYER_KHRONOS_validation");
	}
	auto const layers = vk::enumerateInstanceLayerProperties();
	for (auto szRequiredLayer : data.layers)
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
			LOG_E("[{}] Required layer [{}] not available!", utils::tName<VkInstance>(), szRequiredLayer);
			return false;
		}
	}
	if (!createInstance(std::move(data.extensions), std::move(data.layers)))
	{
		return false;
	}
	if (data.bAddValidationLayers && !setupDebugMessenger())
	{
		return false;
	}
	LOG_I("[{}] Constructed", utils::tName<VkInstance>());
	return true;
}

void VkInstance::destroy()
{
	if (m_instance != vk::Instance())
	{
		if (m_debugMessenger != vk::DebugUtilsMessengerEXT())
		{
			m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger, nullptr, m_loader);
		}
		vkDestroyInstance(m_instance, nullptr);
		m_instance = vk::Instance();
		LOG_I("[{}] Destroyed", utils::tName<VkInstance>());
	}
	return;
}

bool VkInstance::createInstance(std::vector<char const*> const& extensions, std::vector<char const*> const& layers)
{
	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "LittleEngineVk Game";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "LittleEngineVk";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	vk::InstanceCreateInfo createInfo;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();
	createInfo.enabledExtensionCount = (u32)extensions.size();
	createInfo.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();
	createInfo.enabledLayerCount = (u32)layers.size();
	if (vk::createInstance(&createInfo, nullptr, &m_instance) != vk::Result::eSuccess)
	{
		LOG_E("[{}] Failed to create Vulkan instance!", utils::tName<VkInstance>());
		return false;
	}
	return true;
}

bool VkInstance::setupDebugMessenger()
{
	vk::DynamicLoader dl;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	m_loader.init(vkGetInstanceProcAddr);
	m_loader.init(m_instance);
	vk::DebugUtilsMessengerCreateInfoEXT createInfo;
	createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
								 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
	createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
							 | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
	createInfo.pfnUserCallback = &validationCallback;
	m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(createInfo, nullptr, m_loader);
	return m_debugMessenger != vk::DebugUtilsMessengerEXT();
}
} // namespace le
