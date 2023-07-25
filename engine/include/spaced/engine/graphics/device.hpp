#pragma once
#include <spaced/engine/core/ptr.hpp>
#include <spaced/engine/graphics/allocator.hpp>
#include <vulkan/vulkan.hpp>
#include <limits>
#include <mutex>

struct GLFWwindow;

namespace spaced::graphics {
struct RenderDeviceCreateInfo {
	bool validation{};
};

class Device : public MonoInstance<Device> {
  public:
	static constexpr auto max_timeout_v{std::numeric_limits<std::uint64_t>::max()};

	using CreateInfo = RenderDeviceCreateInfo;

	struct Info {
		bool validation{};
		bool portability{};
	};

	explicit Device(Ptr<GLFWwindow> window, CreateInfo const& create_info = {});
	~Device();

	[[nodiscard]] auto instance() const -> vk::Instance { return *m_instance; }
	[[nodiscard]] auto surface() const -> vk::SurfaceKHR { return *m_surface; }
	[[nodiscard]] auto physical_device() const -> vk::PhysicalDevice { return m_physical_device; }
	[[nodiscard]] auto device() const -> vk::Device { return *m_device; }
	[[nodiscard]] auto queue() const -> vk::Queue { return m_queue; }
	[[nodiscard]] auto allocator() const -> Allocator& { return *m_allocator; }
	[[nodiscard]] auto queue_family() const -> std::uint32_t { return m_queue_family; }
	[[nodiscard]] auto mutex() const -> std::mutex& { return m_mutex; }

	auto wait_for(vk::Fence fence, std::uint64_t timeout = max_timeout_v) const -> bool;
	auto reset(vk::Fence fence, bool wait_first = true) const -> bool;

	[[nodiscard]] auto acquire_next_image(vk::SwapchainKHR swapchain, std::uint32_t& out_image_index, vk::Semaphore signal) const -> vk::Result;
	auto submit(vk::SubmitInfo2 const& submit_info, vk::Fence signal = {}) const -> bool;
	auto submit_and_present(vk::SubmitInfo2 const& submit_info, vk::Fence submit_signal, vk::PresentInfoKHR const& present_info) const -> vk::Result;

  private:
	Info m_info{};
	mutable std::mutex m_mutex{};

	vk::UniqueInstance m_instance{};
	vk::UniqueDebugUtilsMessengerEXT m_debug_messenger{};
	vk::UniqueSurfaceKHR m_surface{};
	vk::PhysicalDevice m_physical_device{};
	vk::UniqueDevice m_device{};
	vk::Queue m_queue{};
	std::unique_ptr<Allocator> m_allocator{};

	vk::PhysicalDeviceProperties m_device_properties{};
	std::uint32_t m_queue_family{};
};
} // namespace spaced::graphics
