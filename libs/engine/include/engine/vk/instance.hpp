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
	};

private:
	std::vector<vk::LayerProperties> m_availableLayers;
	vk::Instance m_instance;

public:
	~VkInstance();

	bool init(Data const& data);
	void destroy();

public:
	vk::Instance const& vkInst() const;
};
} // namespace le
