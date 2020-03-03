#include <algorithm>
#include <cstring>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/vk/instance.hpp"

namespace le
{
VkInstance::~VkInstance()
{
	destroy();
}

bool VkInstance::init(Data const& data)
{
	if (m_instance != vk::Instance())
	{
		LOG_W("[{}] already initialised!", utils::tName<VkInstance>());
		return true;
	}
	m_availableLayers = vk::enumerateInstanceLayerProperties();
	for (auto szRequiredLayer : data.layers)
	{
		bool bFound = false;
		for (auto const& layerProperties : m_availableLayers)
		{
			if (std::string_view(szRequiredLayer) == std::string_view(layerProperties.layerName))
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

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "LittleEngineVk Game";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "LittleEngineVk";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	vk::InstanceCreateInfo createInfo;
	createInfo.pApplicationInfo = &appInfo;

	createInfo.ppEnabledExtensionNames = data.extensions.empty() ? nullptr : data.extensions.data();
	createInfo.enabledExtensionCount = (u32)data.extensions.size();
	createInfo.ppEnabledLayerNames = data.layers.empty() ? nullptr : data.layers.data();
	createInfo.enabledLayerCount = (u32)data.layers.size();
	if (vk::createInstance(&createInfo, nullptr, &m_instance) != vk::Result::eSuccess)
	{
		LOG_E("[{}] Failed to create Vulkan instance!", utils::tName<VkInstance>());
		return false;
	}
	LOG_I("[{}] Constructed", utils::tName<VkInstance>());
	return true;
}

void VkInstance::destroy()
{
	if (m_instance != vk::Instance())
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = vk::Instance();
		LOG_I("[{}] Destroyed", utils::tName<VkInstance>());
	}
	return;
}
} // namespace le
