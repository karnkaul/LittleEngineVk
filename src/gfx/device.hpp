#pragma once
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <core/std_types.hpp>
#include <dumb_log/log.hpp>
#include <gfx/common.hpp>
#include <kt/enum_flags/enum_flags.hpp>
#include <kt/result/result.hpp>

namespace le::gfx {
struct Instance final {
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceLimits deviceLimits;

	f32 lineWidthMin = 1.0f;
	f32 lineWidthMax = 1.0f;
	dl::level validationLog = dl::level::warning;

	u32 findMemoryType(u32 typeFilter, vk::MemoryPropertyFlags properties) const;
	f32 lineWidth(f32 desired) const;

	kt::result<vk::Format> supportedFormat(PriorityList<vk::Format> const& desired, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

	template <typename vkType>
	void destroy(vkType object) const;
};

struct Device final {
	struct {
		Queue graphics;
		Queue present;
		Queue transfer;
	} queues;

	vk::Device device;

	bool isValid(vk::SurfaceKHR surface) const;
	std::vector<u32> queueIndices(QFlags flags) const;

	void waitIdle() const;

	vk::Fence createFence(bool bSignalled) const;
	void resetOrCreateFence(vk::Fence& out_fence, bool bSignalled) const;
	void waitFor(vk::Fence optional) const;
	void waitAll(vk::ArrayProxy<vk::Fence const> validFences) const;
	void resetFence(vk::Fence optional) const;
	void resetAll(vk::ArrayProxy<vk::Fence const> validFences) const;

	bool isSignalled(vk::Fence fence) const;
	bool allSignalled(View<vk::Fence const> fences) const;

	vk::ImageView createImageView(ImageViewInfo const& info);

	vk::PipelineLayout createPipelineLayout(vk::ArrayProxy<vk::PushConstantRange const> pushConstants,
											vk::ArrayProxy<vk::DescriptorSetLayout const> setLayouts);

	vk::DescriptorSetLayout createDescriptorSetLayout(vk::ArrayProxy<vk::DescriptorSetLayoutBinding const> bindings);
	vk::DescriptorPool createDescriptorPool(vk::ArrayProxy<vk::DescriptorPoolSize const> poolSizes, u32 maxSets = 1);
	std::vector<vk::DescriptorSet> allocateDescriptorSets(vk::DescriptorPool pool, vk::ArrayProxy<vk::DescriptorSetLayout> layouts, u32 setCount = 1);

	vk::RenderPass createRenderPass(vk::ArrayProxy<vk::AttachmentDescription const> attachments, vk::ArrayProxy<vk::SubpassDescription const> subpasses,
									vk::ArrayProxy<vk::SubpassDependency> dependencies);

	vk::Framebuffer createFramebuffer(vk::RenderPass renderPass, vk::ArrayProxy<vk::ImageView const> attachments, vk::Extent2D extent, u32 layers = 1);

	template <typename vkType>
	void destroy(vkType object) const;

	template <typename vkType, typename... vkTypes>
	void destroy(vkType object, vkTypes... objects) const;
};

inline Instance g_instance;
inline Device g_device;

struct Service final {
	Service(InitInfo const& info);
	~Service();
};

template <typename vkType>
void Instance::destroy(vkType object) const {
	if (object != vkType() && instance != vk::Instance()) {
		instance.destroy(object);
	}
}

template <typename vkType>
void Device::destroy(vkType object) const {
	if (device != vk::Device()) {
		if constexpr (std::is_same_v<vkType, vk::DescriptorSetLayout>) {
			if (object != vk::DescriptorSetLayout()) {
				device.destroyDescriptorSetLayout(object);
			}
		} else if constexpr (std::is_same_v<vkType, vk::DescriptorPool>) {
			if (object != vk::DescriptorPool()) {
				device.destroyDescriptorPool(object);
			}
		} else if constexpr (std::is_same_v<vkType, vk::ImageView>) {
			if (object != vk::ImageView()) {
				device.destroyImageView(object);
			}
		} else if constexpr (std::is_same_v<vkType, vk::Sampler>) {
			if (object != vk::Sampler()) {
				device.destroySampler(object);
			}
		} else {
			if (object != vkType()) {
				device.destroy(object);
			}
		}
	}
	return;
}

template <typename vkType, typename... vkTypes>
void Device::destroy(vkType object, vkTypes... objects) const {
	destroy(object);
	destroy(objects...);
}
} // namespace le::gfx
