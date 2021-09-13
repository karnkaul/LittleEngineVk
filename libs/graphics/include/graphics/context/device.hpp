#pragma once
#include <core/not_null.hpp>
#include <graphics/context/defer_queue.hpp>
#include <graphics/context/instance.hpp>
#include <graphics/context/queue_multiplex.hpp>
#include <graphics/utils/layout_state.hpp>

namespace le::graphics {
class Device final : public Pinned {
  public:
	template <typename T>
	using vAP = vk::ArrayProxy<T const> const&;

	template <typename T>
	struct Deleter;

	enum class QSelect { eOptimal, eSingleFamily, eSingleQueue };
	inline static constexpr std::array<std::string_view, 2> requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME};

	struct CreateInfo;

	Device(not_null<Instance*> instance, vk::SurfaceKHR surface, CreateInfo const& info);
	~Device();

	static constexpr vk::BufferUsageFlagBits bufferUsage(vk::DescriptorType type) noexcept;

	void waitIdle();
	bool valid(vk::SurfaceKHR surface) const;

	vk::Semaphore makeSemaphore() const;
	vk::Fence makeFence(bool bSignalled) const;
	void resetOrMakeFence(vk::Fence& out_fence, bool bSignalled) const;
	void waitFor(vk::Fence optional) const;
	void waitAll(vAP<vk::Fence> validFences) const;
	void resetFence(vk::Fence optional) const;
	void resetAll(vAP<vk::Fence> validFences) const;

	bool signalled(Span<vk::Fence const> fences) const;

	vk::CommandPool makeCommandPool(vk::CommandPoolCreateFlags flags, QType qtype) const;
	vk::ImageView makeImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
								vk::ImageViewType type = vk::ImageViewType::e2D) const;

	vk::PipelineCache makePipelineCache() const;
	vk::PipelineLayout makePipelineLayout(vAP<vk::PushConstantRange> pushConstants, vAP<vk::DescriptorSetLayout> setLayouts) const;

	vk::DescriptorSetLayout makeDescriptorSetLayout(vAP<vk::DescriptorSetLayoutBinding> bindings) const;
	vk::DescriptorPool makeDescriptorPool(vAP<vk::DescriptorPoolSize> poolSizes, u32 maxSets = 1) const;
	std::vector<vk::DescriptorSet> allocateDescriptorSets(vk::DescriptorPool pool, vAP<vk::DescriptorSetLayout> layouts, u32 setCount = 1) const;

	vk::RenderPass makeRenderPass(vAP<vk::AttachmentDescription> attachments, vAP<vk::SubpassDescription> subpasses,
								  vAP<vk::SubpassDependency> dependencies = {}) const;

	vk::Framebuffer makeFramebuffer(vk::RenderPass renderPass, vAP<vk::ImageView> attachments, vk::Extent2D extent, u32 layers = 1) const;
	vk::Sampler makeSampler(vk::SamplerCreateInfo info) const;

	bool setDebugUtilsName(vk::DebugUtilsObjectNameInfoEXT const& info) const;
	bool setDebugUtilsName(u64 handle, vk::ObjectType type, std::string_view name) const;

	template <typename T, typename... Args>
	T make(Args&&... args);
	template <typename T, typename... Ts>
	void destroy(T& out_t, Ts&... out_ts);
	void defer(DeferQueue::Callback const& callback, Buffering defer = DeferQueue::defaultDefer);

	void decrementDeferred();

	template <typename T>
	static constexpr bool default_v(T const& t) noexcept {
		return t == T();
	}

	PhysicalDevice const& physicalDevice() const noexcept { return m_physicalDevice; }
	QueueMultiplex& queues() noexcept { return m_queues; }
	QueueMultiplex const& queues() const noexcept { return m_queues; }
	vk::Device device() const noexcept { return m_device; }
	vk::SurfaceKHR surface() const noexcept { return m_metadata.surface; }
	TPair<f32> lineWidthLimit() const noexcept { return m_metadata.lineWidth; }

	LayoutState m_layouts;
	not_null<Instance*> m_instance;

  private:
	CommandBuffer beginCmd();
	void endCmd(CommandBuffer cb);

	PhysicalDevice m_physicalDevice;
	vk::Device m_device;
	DeferQueue m_deferred;
	QueueMultiplex m_queues;

	struct {
		std::vector<char const*> extensions;
		vk::SurfaceKHR surface;
		ktl::fixed_vector<PhysicalDevice, 8> available;
		vk::PhysicalDeviceLimits limits;
		TPair<f32> lineWidth;
	} m_metadata;
};

struct Device::CreateInfo {
	Span<std::string_view const> extensions = requiredExtensions;
	DevicePicker* pPicker = nullptr;
	std::optional<std::size_t> pickOverride;
	QSelect qselect = QSelect::eOptimal;
};

// impl

template <typename T>
struct Device::Deleter {
	void operator()(not_null<Device*> device, T t) const { device->destroy(t); }
};

constexpr vk::BufferUsageFlagBits Device::bufferUsage(vk::DescriptorType type) noexcept {
	switch (type) {
	case vk::DescriptorType::eStorageBuffer: return vk::BufferUsageFlagBits::eStorageBuffer;
	default: return vk::BufferUsageFlagBits::eUniformBuffer;
	}
}

template <typename T, typename... Args>
T Device::make(Args&&... args) {
	if constexpr (std::is_same_v<T, vk::Semaphore>) {
		return makeSemaphore(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::Fence>) {
		return makeFence(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::ImageView>) {
		return makeImageView(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::PipelineLayout>) {
		return makePipelineLayout(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::DescriptorSetLayout>) {
		return makeDescriptorSetLayout(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::DescriptorPool>) {
		return makeDescriptorPool(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::RenderPass>) {
		return makeRenderPass(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::Framebuffer>) {
		return makeFramebuffer(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::CommandPool>) {
		return makeCommandPool(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::PipelineCache>) {
		return makePipelineCache(std::forward<Args>(args)...);
	} else if constexpr (std::is_same_v<T, vk::Sampler>) {
		return makeSampler(std::forward<Args>(args)...);
	} else {
		static_assert(false_v<T>, "Invalid type");
	}
}

template <typename T, typename... Ts>
void Device::destroy(T& out_t, Ts&... out_ts) {
	if constexpr (std::is_same_v<T, vk::Instance> || std::is_same_v<T, vk::Device>) {
		out_t.destroy();
	} else {
		if (!default_v(m_device) && !default_v(m_instance->m_instance) && !default_v(out_t)) {
			if constexpr (std::is_same_v<T, vk::SurfaceKHR>) {
				m_instance->m_instance.destroySurfaceKHR(out_t);
			} else if constexpr (std::is_same_v<T, vk::DebugUtilsMessengerEXT>) {
				m_instance->m_instance.destroy(out_t, nullptr);
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
	if constexpr (sizeof...(Ts) > 0) { destroy(out_ts...); }
}
} // namespace le::graphics
