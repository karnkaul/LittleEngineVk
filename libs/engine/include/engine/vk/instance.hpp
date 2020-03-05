#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "device.hpp"

namespace le
{
class VkInstance final
{
public:
	struct Data
	{
		std::vector<char const*> extensions;
		std::vector<char const*> layers;
		bool bAddValidationLayers = false;
	};

private:
	struct QueueFamilyIndices
	{
		std::optional<u32> graphics;
		std::optional<u32> presentation;
	};

public:
	static std::string const s_tName;

private:
	vk::Instance m_instance;
	vk::DispatchLoaderDynamic m_loader;
	vk::DebugUtilsMessengerEXT m_debugMessenger;
	std::vector<VkDevice> m_devices;
	VkDevice* m_pDevice = nullptr;
	QueueFamilyIndices m_queueFamilyIndices;

public:
	~VkInstance();

	bool init(Data data);
	void destroy();

public:
	bool isInit() const;
	vk::Instance const& vkInst() const;
	VkDevice const* device() const;
	VkDevice* device();

private:
	bool createInstance(std::vector<char const*> const& extensions, std::vector<char const*> const& layers);
	bool setupDebugMessenger();
	bool setupDevices(std::vector<char const*> const& layers);
};
} // namespace le
