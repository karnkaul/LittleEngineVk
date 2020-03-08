#pragma once
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "device.hpp"

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
	std::vector<char const*> m_layers;
	std::unique_ptr<class Device> m_uDevice;

public:
	Instance();
	~Instance();

public:
	Device* device() const;
	vk::DispatchLoaderDynamic const& vkLoader() const;

public:
	explicit operator vk::Instance const&() const;
	explicit operator VkInstance() const;

	template <typename vkType>
	void destroy(vkType const& handle) const;

private:
	bool createInstance(std::vector<char const*> const& extensions);
	bool setupDebugMessenger();
};

template <typename vkType>
void Instance::destroy(vkType const& handle) const
{
	if (handle != vkType())
	{
		m_instance.destroy(handle);
	}
	return;
}
} // namespace le::vuk
