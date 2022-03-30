#pragma once
#include <ktl/async/kfunction.hpp>
#include <ktl/fixed_pimpl.hpp>
#include <levk/core/os.hpp>
#include <levk/core/log.hpp>
#include <levk/core/time.hpp>
#include <levk/core/version.hpp>
#include <levk/graphics/device/defer_queue.hpp>
#include <levk/graphics/device/physical_device.hpp>
#include <levk/graphics/device/queue.hpp>
#include <levk/graphics/utils/layout_state.hpp>

namespace le::graphics {
enum class Validation { eOn, eOff };

class Device final : public Pinned {
  public:
	using MakeSurface = ktl::kfunction<vk::SurfaceKHR(vk::Instance)>;

	template <typename T>
	using vAP = vk::ArrayProxy<T const> const&;

	template <typename T>
	struct Deleter;

	enum class QSelect { eOptimal, eSingleFamily, eSingleQueue };
	static constexpr std::string_view requiredExtensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#if defined(LEVK_OS_APPLE)
		"VK_KHR_portability_subset"
#endif
	};
	static constexpr stdch::nanoseconds fenceWait = 1s;

	struct CreateInfo;

	static ktl::fixed_vector<PhysicalDevice, 8> physicalDevices();

	static std::unique_ptr<Device> make(CreateInfo const& info, MakeSurface&& makeSurface);
	~Device();

	static constexpr vk::BufferUsageFlagBits bufferUsage(vk::DescriptorType type) noexcept;

	void waitIdle();
	bool valid(vk::SurfaceKHR surface) const;

	vk::UniqueSurfaceKHR makeSurface() const;
	vk::Semaphore makeSemaphore() const;
	vk::Fence makeFence(bool signalled) const;
	void resetOrMakeFence(vk::Fence& out_fence, bool signalled) const;
	bool isBusy(vk::Fence fence) const;
	void waitFor(Span<vk::Fence const> fences, stdch::nanoseconds wait = fenceWait) const;
	void resetFence(vk::Fence fence, bool wait) const;
	void resetAll(Span<vk::Fence const> fences) const;
	void resetCommandPool(vk::CommandPool pool) const;

	bool signalled(Span<vk::Fence const> fences) const;

	vk::ImageView makeImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
								vk::ImageViewType type = vk::ImageViewType::e2D, u32 mipLevels = 1U) const;

	vk::PipelineCache makePipelineCache() const;
	vk::PipelineLayout makePipelineLayout(vAP<vk::PushConstantRange> pushConstants, vAP<vk::DescriptorSetLayout> setLayouts) const;

	vk::DescriptorSetLayout makeDescriptorSetLayout(vAP<vk::DescriptorSetLayoutBinding> bindings) const;
	vk::DescriptorPool makeDescriptorPool(Span<vk::DescriptorPoolSize const> poolSizes, u32 maxSets = 1) const;
	std::vector<vk::DescriptorSet> allocateDescriptorSets(vk::DescriptorPool pool, vAP<vk::DescriptorSetLayout> layouts, u32 setCount = 1) const;

	vk::Framebuffer makeFramebuffer(vk::RenderPass renderPass, Span<vk::ImageView const> attachments, vk::Extent2D extent, u32 layers = 1) const;
	vk::Sampler makeSampler(vk::SamplerCreateInfo info) const;

	bool setDebugUtilsName(vk::DebugUtilsObjectNameInfoEXT const& info) const;
	bool setDebugUtilsName(u64 handle, vk::ObjectType type, std::string_view name) const;

	template <typename T, typename... Args>
	T make(Args&&... args);
	template <typename T, typename... Ts>
	void destroy(T& out_t, Ts&... out_ts);
	void defer(DeferQueue::Callback&& callback, Buffering defer = DeferQueue::defaultDefer);

	void decrementDeferred();

	template <typename T>
	static constexpr bool default_v(T const& t) noexcept {
		return t == T();
	}

	PhysicalDevice const& physicalDevice() const noexcept { return m_metadata.available[m_physicalDeviceIndex]; }
	Queues const& queues() const noexcept { return m_queues; }
	vk::Instance instance() const noexcept { return *m_instance; }
	vk::Device device() const noexcept { return *m_device; }
	TPair<f32> lineWidthLimit() const noexcept { return {m_metadata.limits.lineWidthRange[0], m_metadata.limits.lineWidthRange[1]}; }
	f32 maxAnisotropy() const noexcept { return m_metadata.limits.maxSamplerAnisotropy; }

	LayoutState m_layouts;

  private:
	struct Impl;
	Device(Impl&&) noexcept;

	ktl::fixed_pimpl<Impl, 16> m_impl;
	MakeSurface m_makeSurface;
	vk::UniqueInstance m_instance;
	vk::UniqueDebugUtilsMessengerEXT m_messenger;
	vk::UniqueDevice m_device;
	DeferQueue m_deferred;
	Queues m_queues;
	std::size_t m_physicalDeviceIndex{};

	struct {
		std::vector<char const*> extensions;
		ktl::fixed_vector<PhysicalDevice, 8> available;
		vk::PhysicalDeviceLimits limits;
	} m_metadata;

	friend class FontFace;
};

struct Device::CreateInfo {
	struct {
		Span<std::string_view const> extensions;
		Validation validation = Validation::eOff;
	} instance;
	struct {
		std::string_view name;
		Version version;
	} app;

	Span<std::string_view const> extensions = requiredExtensions;
	std::string_view customDeviceName;
	LogLevel validationLogLevel = LogLevel::info;
	QSelect qselect = QSelect::eOptimal;
};

// impl

template <typename T>
struct Device::Deleter {
	void operator()(vk::Device device, T t) const { device.destroy(t); }
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
	if (!default_v(out_t)) {
		m_device->destroy(out_t);
		out_t = T();
	}
	if constexpr (sizeof...(Ts) > 0) { destroy(out_ts...); }
}
} // namespace le::graphics
