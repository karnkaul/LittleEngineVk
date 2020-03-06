#include <algorithm>
#include <cstring>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/vk/instance.hpp"
#include "engine/vk/instanceImpl.hpp"

namespace le::vuk
{
namespace
{
VKAPI_ATTR VkBool32 VKAPI_CALL validationCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
												  VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void*)
{
	static std::string_view const name = "vk::validation";
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		LOG_E("[{}] {}", name, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		LOG_W("[{}] {}", name, pCallbackData->pMessage);
		break;
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		LOG_I("[{}] {}", name, pCallbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		LOG_D("[{}] {}", name, pCallbackData->pMessage);
		break;
	}
	return VK_FALSE;
}
} // namespace

Instance::Service::Service()
{
	if (!g_instance.init(g_instanceData))
	{
		throw std::runtime_error("Failed to create vk::Instance!");
	}
}

Instance::Service::~Service()
{
	g_instance.deinit();
}

std::string const Instance::s_tName = utils::tName<Instance>();

Instance::~Instance()
{
	ASSERT(m_instance == vk::Instance(), "Instance not deinitialised!");
}

bool Instance::init(Data data)
{
	if (isInit())
	{
		LOG_W("[{}] already initialised!", s_tName);
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
			LOG_E("[{}] Required layer [{}] not available!", s_tName, szRequiredLayer);
			return false;
		}
	}
	m_layers = std::move(data.layers);
	if (!createInstance(data.extensions))
	{
		return false;
	}
	vk::DynamicLoader dl;
	m_loader.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	m_loader.init(m_instance);
	if (data.bAddValidationLayers && !setupDebugMessenger())
	{
		return false;
	}
	if (!getPhysicalDevices())
	{
		return false;
	}
	LOG_I("[{}] Constructed", s_tName);
	return true;
}

void Instance::deinit()
{
	if (isInit())
	{
		m_physicalDevices.clear();
		if (m_instance != vk::Instance())
		{
			if (m_debugMessenger != vk::DebugUtilsMessengerEXT())
			{
				m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger, nullptr, m_loader);
			}
			m_instance.destroy();
			m_instance = vk::Instance();
			LOG_I("[{}] Destroyed", s_tName);
		}
	}
	return;
}

bool Instance::isInit() const
{
	return m_instance != vk::Instance();
}

PhysicalDevice const* Instance::activeDevice() const
{
	return m_pPhysicalDevice;
}

PhysicalDevice* Instance::activeDevice()
{
	return m_pPhysicalDevice;
}

vk::DispatchLoaderDynamic const& Instance::vkLoader() const
{
	return m_loader;
}

Instance::operator vk::Instance const&() const
{
	return m_instance;
}

Instance::operator VkInstance() const
{
	return m_instance;
}

void Instance::destroy(vk::SurfaceKHR const& surface)
{
	m_instance.destroy(surface);
}

bool Instance::createInstance(std::vector<char const*> const& extensions)
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
	createInfo.ppEnabledLayerNames = m_layers.empty() ? nullptr : m_layers.data();
	createInfo.enabledLayerCount = (u32)m_layers.size();
	if (vk::createInstance(&createInfo, nullptr, &m_instance) != vk::Result::eSuccess)
	{
		LOG_E("[{}] Failed to create Vulkan instance!", s_tName);
		return false;
	}
	return true;
}

bool Instance::setupDebugMessenger()
{
	vk::DebugUtilsMessengerCreateInfoEXT createInfo;
	createInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
								 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
	createInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
							 | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
	createInfo.pfnUserCallback = &validationCallback;
	m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(createInfo, nullptr, m_loader);
	return m_debugMessenger != vk::DebugUtilsMessengerEXT();
}

bool Instance::getPhysicalDevices()
{
	auto devices = m_instance.enumeratePhysicalDevices();
	m_physicalDevices.reserve(devices.size());
	for (auto const& device : devices)
	{
		m_physicalDevices.emplace_back(device);
		if (m_physicalDevices.back().m_type == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			m_pPhysicalDevice = &m_physicalDevices.back();
		}
	}
	if (!m_pPhysicalDevice && !m_physicalDevices.empty())
	{
		m_pPhysicalDevice = &m_physicalDevices.front();
	}
	if (m_pPhysicalDevice && m_pPhysicalDevice->m_graphicsQueueFamilyIndex.has_value())
	{
		LOG_D("[{}] GPU: {}", PhysicalDevice::s_tName, m_pPhysicalDevice->m_name);
		return true;
	}
	LOG_E("[{}] Failed to find a physical device!", s_tName);
	return false;
}
} // namespace le::vuk
