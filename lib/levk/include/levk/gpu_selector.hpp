#pragma once
#include <levk/core/polymorphic.hpp>
#include <vulkan/vulkan.hpp>
#include <span>

namespace levk {
/// \brief Customizable Physical Device selector.
class IGpuSelector : public Polymorphic {
  public:
	virtual auto select_device(std::span<vk::PhysicalDevice const> devices) -> vk::PhysicalDevice = 0;
};
} // namespace levk
