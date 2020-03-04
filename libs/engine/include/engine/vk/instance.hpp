#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"

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
	vk::Instance m_instance;
	vk::DispatchLoaderDynamic m_loader;
	vk::DebugUtilsMessengerEXT m_debugMessenger;

public:
	~VkInstance();

	bool init(Data data);
	void destroy();

public:
	vk::Instance const& vkInst() const;

private:
	bool createInstance(std::vector<char const*> const& extensions, std::vector<char const*> const& layers);
	bool setupDebugMessenger();
};
} // namespace le
