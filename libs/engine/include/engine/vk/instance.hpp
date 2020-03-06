#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "physical_device.hpp"

namespace le::vuk
{
class Instance final
{
public:
	struct Data final
	{
		std::vector<char const*> extensions;
		std::vector<char const*> layers;
		bool bAddValidationLayers = false;
	};
	struct Service final
	{
		Service();
		~Service();
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
	std::vector<PhysicalDevice> m_physicalDevices;
	vuk::PhysicalDevice* m_pPhysicalDevice = nullptr;
	QueueFamilyIndices m_queueFamilyIndices;
	std::vector<char const*> m_layers;

public:
	~Instance();

	bool init(Data data);
	void deinit();

public:
	bool isInit() const;
	PhysicalDevice const* activeDevice() const;
	PhysicalDevice* activeDevice();
	vk::DispatchLoaderDynamic const& vkLoader() const;

public:
	operator vk::Instance const&() const;
	operator VkInstance() const;
	void destroy(vk::SurfaceKHR const& surface);

private:
	bool createInstance(std::vector<char const*> const& extensions);
	bool setupDebugMessenger();
	bool getPhysicalDevices();

private:
	friend class Device;
};
} // namespace le::vuk
