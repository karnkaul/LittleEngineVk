#pragma once
#include <core/log.hpp>
#include <graphics/context/defer_queue.hpp>
#include <graphics/context/physical_device.hpp>
#include <graphics/context/queue.hpp>
#include <graphics/utils/layout_state.hpp>
#include <ktl/move_only_function.hpp>

namespace le::graphics {
enum class Validation { eOn, eOff };

class Device final : public Pinned {
  public:
	using MakeSurface = ktl::move_only_function<vk::SurfaceKHR(vk::Instance)>;

	template <typename T>
	using vAP = vk::ArrayProxy<T const> const&;

	template <typename T>
	struct Deleter;

	enum class QSelect { eOptimal, eSingleFamily, eSingleQueue };
	inline static constexpr std::array<std::string_view, 2> requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME};

	struct CreateInfo;

	static ktl::fixed_vector<PhysicalDevice, 8> physicalDevices();

	Device(CreateInfo const& info, MakeSurface&& makeSurface);
	~Device();

	static constexpr vk::BufferUsageFlagBits bufferUsage(vk::DescriptorType type) noexcept;

	void waitIdle();
	bool valid(vk::SurfaceKHR surface) const;

	vk::UniqueSurfaceKHR makeSurface() const;
	vk::Semaphore makeSemaphore() const;
	vk::Fence makeFence(bool bSignalled) const;
	void resetOrMakeFence(vk::Fence& out_fence, bool bSignalled) const;
	void waitFor(vk::Fence optional) const;
	void waitAll(vAP<vk::Fence> validFences) const;
	void resetFence(vk::Fence optional) const;
	void resetAll(vAP<vk::Fence> validFences) const;
	void resetCommandPool(vk::CommandPool pool) const;

	bool signalled(Span<vk::Fence const> fences) const;

	vk::ImageView makeImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
								vk::ImageViewType type = vk::ImageViewType::e2D, u32 mipLevels = 1U) const;

	vk::PipelineCache makePipelineCache() const;
	vk::PipelineLayout makePipelineLayout(vAP<vk::PushConstantRange> pushConstants, vAP<vk::DescriptorSetLayout> setLayouts) const;

	vk::DescriptorSetLayout makeDescriptorSetLayout(vAP<vk::DescriptorSetLayoutBinding> bindings) const;
	vk::DescriptorPool makeDescriptorPool(vAP<vk::DescriptorPoolSize> poolSizes, u32 maxSets = 1) const;
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
	TPair<f32> lineWidthLimit() const noexcept { return m_metadata.lineWidth; }
	f32 maxAnisotropy() const noexcept { return m_metadata.anisotropy; }

	LayoutState m_layouts;

  private:
	CommandBuffer beginCmd();
	void endCmd(CommandBuffer cb);

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
		TPair<f32> lineWidth{};
		f32 anisotropy{};
	} m_metadata;
};

struct Device::CreateInfo {
	struct {
		Span<std::string_view const> extensions;
		Validation validation = Validation::eOff;
	} instance;

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
