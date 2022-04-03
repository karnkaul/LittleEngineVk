#pragma once
#include <ktl/async/kfunction.hpp>
#include <ktl/fixed_pimpl.hpp>
#include <levk/core/log.hpp>
#include <levk/core/os.hpp>
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

	template <typename T, typename Del = Deleter<T>>
	class Unique;

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

	template <typename... T>
	void destroy(T const&... t);
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
	void operator()(Device& device, T t) const { device.destroy(t); }
};

template <typename T, typename Del>
class Device::Unique {
  public:
	Unique() = default;
	Unique(T t, not_null<Device*> device) noexcept : m_t(std::move(t)), m_device(device) {}
	Unique(Unique&& rhs) noexcept : Unique() { exchg(*this, rhs); }
	Unique& operator=(Unique&& rhs) noexcept { return (exchg(*this, rhs), *this); }
	~Unique() {
		if (m_device && m_t != T{}) { Del{}(*m_device, m_t); }
	}

	explicit operator bool() const noexcept { return m_device != nullptr && m_t != T{}; }
	T const& operator*() const noexcept { return get(); }
	T const* operator->() const noexcept { return &get(); }

	T const& get() const noexcept { return m_t; }
	T& get() noexcept { return m_t; }

  private:
	static void exchg(Unique& lhs, Unique& rhs) noexcept {
		std::swap(lhs.m_t, rhs.m_t);
		std::swap(lhs.m_device, rhs.m_device);
	}

	T m_t;
	Opt<Device> m_device{};
};

template <typename Del>
class Device::Unique<void, Del> {
  public:
	Unique(Opt<Device> device = {}) noexcept : m_device(device) {}
	Unique(Unique&& rhs) noexcept : Unique() { exchg(*this, rhs); }
	Unique& operator=(Unique&& rhs) noexcept { return (exchg(*this, rhs), *this); }
	~Unique() {
		if (m_device) { Del{}(*m_device); }
	}

	explicit operator bool() const noexcept { return m_device != nullptr; }

  private:
	static void exchg(Unique& lhs, Unique& rhs) noexcept { std::swap(lhs.m_device, rhs.m_device); }

	Opt<Device> m_device{};
};

constexpr vk::BufferUsageFlagBits Device::bufferUsage(vk::DescriptorType type) noexcept {
	switch (type) {
	case vk::DescriptorType::eStorageBuffer: return vk::BufferUsageFlagBits::eStorageBuffer;
	default: return vk::BufferUsageFlagBits::eUniformBuffer;
	}
}

template <typename... T>
void Device::destroy(T const&... t) {
	auto d = [](vk::Device device, auto const& a) {
		if (a) { device.destroy(a); }
	};
	(d(*m_device, t), ...);
}
} // namespace le::graphics
