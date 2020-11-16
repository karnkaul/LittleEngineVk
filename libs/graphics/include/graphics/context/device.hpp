#pragma once
#include <memory>
#include <optional>
#include <core/traits.hpp>
#include <graphics/context/defer_queue.hpp>
#include <graphics/context/instance.hpp>
#include <graphics/context/queues.hpp>

namespace le::graphics {
class Device final {
  public:
	static constexpr std::array requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME};

	struct CreateInfo;

	Device(Instance& instance, vk::SurfaceKHR surface, CreateInfo const& info);
	~Device();

	std::vector<AvailableDevice> availableDevices() const;

	void waitIdle();
	bool valid(vk::SurfaceKHR surface) const;

	void waitIdle() const;

	vk::Semaphore createSemaphore() const;
	vk::Fence createFence(bool bSignalled) const;
	void resetOrCreateFence(vk::Fence& out_fence, bool bSignalled) const;
	void waitFor(vk::Fence optional) const;
	void waitAll(vAP<vk::Fence> validFences) const;
	void resetFence(vk::Fence optional) const;
	void resetAll(vAP<vk::Fence> validFences) const;

	bool signalled(Span<vk::Fence> fences) const;

	vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
								  vk::ImageViewType type = vk::ImageViewType::e2D) const;

	vk::PipelineLayout createPipelineLayout(vAP<vk::PushConstantRange> pushConstants, vAP<vk::DescriptorSetLayout> setLayouts) const;

	vk::DescriptorSetLayout createDescriptorSetLayout(vAP<vk::DescriptorSetLayoutBinding> bindings) const;
	vk::DescriptorPool createDescriptorPool(vAP<vk::DescriptorPoolSize> poolSizes, u32 maxSets = 1) const;
	std::vector<vk::DescriptorSet> allocateDescriptorSets(vk::DescriptorPool pool, vAP<vk::DescriptorSetLayout> layouts, u32 setCount = 1) const;

	vk::RenderPass createRenderPass(vAP<vk::AttachmentDescription> attachments, vAP<vk::SubpassDescription> subpasses,
									vAP<vk::SubpassDependency> dependencies) const;

	vk::Framebuffer createFramebuffer(vk::RenderPass renderPass, vAP<vk::ImageView> attachments, vk::Extent2D extent, u32 layers = 1) const;

	template <typename T, typename... Args>
	T construct(Args&&... args);
	template <typename T, typename... Ts>
	void destroy(T& out_t, Ts&... out_ts);
	void defer(Deferred::Callback callback, u64 defer = Deferred::defaultDefer);

	void decrementDeferred();

	Ref<Instance> m_instance;
	vk::Device m_device;
	vk::PhysicalDevice m_physicalDevice;
	DeferQueue m_deferred;
	Queues m_queues;

	struct {
		AvailableDevice picked;
		vk::SurfaceKHR surface;
		std::vector<AvailableDevice> available;
		vk::PhysicalDeviceLimits limits;
		std::pair<f32, f32> lineWidth;
	} m_metadata;

  private:
	std::vector<std::unique_ptr<Queues>> m_queueStorage;
};

struct Device::CreateInfo {
	std::function<AvailableDevice(Span<AvailableDevice>)> pickDevice;
	bool bDedicatedTransfer = true;
	bool bPrintAvailable = false;
};

// impl

template <typename T, typename... Args>
T Device::construct(Args&&... args) {
	if constexpr (std::is_same_v<T, vk::Semaphore>) {
		return createSemaphore(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::Fence>) {
		return createFence(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::ImageView>) {
		return createImageView(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::PipelineLayout>) {
		return createPipelineLayout(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::DescriptorSetLayout>) {
		return createDescriptorSetLayout(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::DescriptorPool>) {
		return createDescriptorPool(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::RenderPass>) {
		return createRenderPass(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::Framebuffer>) {
		return createFramebuffer(std::forward<Args>(args)...);
	} else {
		static_assert(false_v<T>, "Invalid type");
	}
}

template <typename T, typename... Ts>
void Device::destroy(T& out_t, Ts&... out_ts) {
	if constexpr (std::is_same_v<T, vk::Instance> || std::is_same_v<T, vk::Device>) {
		out_t.destroy();
	} else {
		Instance& inst = m_instance;
		if (!default_v(m_device) && !default_v(inst.m_instance) && !default_v(out_t)) {
			if constexpr (std::is_same_v<T, vk::SurfaceKHR>) {
				inst.m_instance.destroySurfaceKHR(out_t);
			} else if constexpr (std::is_same_v<T, vk::DebugUtilsMessengerEXT>) {
				inst.m_instance.destroy(out_t, nullptr, inst.m_loader);
			} else if constexpr (std::is_same_v<T, vk::DescriptorSetLayout>) {
				m_device.destroyDescriptorSetLayout(out_t);
			} else if constexpr (std::is_same_v<T, vk::DescriptorPool>) {
				m_device.destroyDescriptorPool(out_t);
			} else if constexpr (std::is_same_v<T, vk::ImageView>) {
				m_device.destroyImageView(out_t);
			} else if constexpr (std::is_same_v<T, vk::Sampler>) {
				m_device.destroySampler(out_t);
			} else {
				m_device.destroy(out_t);
			}
			out_t = T();
		}
	}
	if constexpr (sizeof...(Ts) > 0) {
		destroy(out_ts...);
	}
}
} // namespace le::graphics
