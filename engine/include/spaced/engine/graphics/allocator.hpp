#pragma once
#include <vk_mem_alloc.h>
#include <spaced/engine/core/mono_instance.hpp>
#include <vulkan/vulkan.hpp>
#include <memory>

namespace spaced::graphics {
class Allocator : public MonoInstance<Allocator> {
  public:
	Allocator(Allocator const&) = delete;
	Allocator(Allocator&&) = delete;
	auto operator=(Allocator const&) -> Allocator& = delete;
	auto operator=(Allocator&&) -> Allocator& = delete;

	Allocator(vk::Instance instance, vk::PhysicalDevice gpu, vk::Device device);
	~Allocator();

	[[nodiscard]] auto device() const -> vk::Device { return m_device; }
	[[nodiscard]] auto vma() const -> VmaAllocator { return m_allocator; }

	operator VmaAllocator() const { return vma(); }

  private:
	vk::Device m_device{};
	VmaAllocator m_allocator{};
};
} // namespace spaced::graphics
