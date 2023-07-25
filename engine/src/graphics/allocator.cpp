#include <spaced/engine/graphics/allocator.hpp>

namespace spaced::graphics {
Allocator::Allocator(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device device) : m_device(device) {
	auto vaci = VmaAllocatorCreateInfo{};
	vaci.instance = instance;
	vaci.physicalDevice = physical_device;
	vaci.device = device;
	auto dl = VULKAN_HPP_DEFAULT_DISPATCHER;
	auto vkFunc = VmaVulkanFunctions{};
	vkFunc.vkGetInstanceProcAddr = dl.vkGetInstanceProcAddr;
	vkFunc.vkGetDeviceProcAddr = dl.vkGetDeviceProcAddr;
	vaci.pVulkanFunctions = &vkFunc;
	if (vmaCreateAllocator(&vaci, &m_allocator) != VK_SUCCESS) { throw Error{"Failed to create Vulkan Allocator"}; }
}

Allocator::~Allocator() { vmaDestroyAllocator(m_allocator); }
} // namespace spaced::graphics
