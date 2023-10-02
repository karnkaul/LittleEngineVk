#pragma once
#include <le/core/ptr.hpp>
#include <le/graphics/allocator.hpp>
#include <le/graphics/font/font_library.hpp>
#include <vulkan/vulkan.hpp>
#include <limits>
#include <mutex>

struct GLFWwindow;

namespace le::graphics {
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

	Device(Device const&) = delete;
	Device(Device&&) = delete;
	auto operator=(Device const&) -> Device& = delete;
	auto operator=(Device&&) -> Device& = delete;

	explicit Device(Ptr<GLFWwindow> window, CreateInfo const& create_info = {});
	~Device();

	[[nodiscard]] auto get_instance() const -> vk::Instance { return *m_instance; }
	[[nodiscard]] auto get_surface() const -> vk::SurfaceKHR { return *m_surface; }
	[[nodiscard]] auto get_physical_device() const -> vk::PhysicalDevice { return m_physical_device; }
	[[nodiscard]] auto get_device() const -> vk::Device { return *m_device; }
	[[nodiscard]] auto get_queue() const -> vk::Queue { return m_queue; }
	[[nodiscard]] auto get_allocator() const -> Allocator& { return *m_allocator; }
	[[nodiscard]] auto get_queue_family() const -> std::uint32_t { return m_queue_family; }

	[[nodiscard]] auto get_font_library() const -> FontLibrary& { return *m_font_library; }

	[[nodiscard]] auto get_info() const -> Info const& { return m_info; }

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
	std::unique_ptr<FontLibrary> m_font_library{FontLibrary::make()};

	vk::PhysicalDeviceProperties m_device_properties{};
	std::uint32_t m_queue_family{};
};
} // namespace le::graphics
