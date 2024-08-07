#pragma once
#include <glm/vec2.hpp>
#include <levk/bitmap.hpp>
#include <levk/core/c_string.hpp>
#include <levk/core/polymorphic.hpp>
#include <vulkan/vulkan.hpp>
#include <span>

namespace levk {
struct ImageCreateInfo {
	static constexpr auto usage_v = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

	vk::Extent2D extent{1, 1};
	vk::Format format{vk::Format::eR8G8B8A8Srgb};
	vk::ImageUsageFlags usage{usage_v};
	vk::ImageAspectFlagBits aspect{vk::ImageAspectFlagBits::eColor};
	vk::ImageTiling tiling{vk::ImageTiling::eOptimal};
	vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
	vk::ImageViewType view_type{vk::ImageViewType::e2D};
	bool mip_map{true};
};

struct RenderImageInfo {
	vk::Image image{};
	vk::ImageView view{};
	vk::ImageViewType type{};
	vk::Extent2D extent{};
	vk::Format format{};
	vk::ImageLayout layout{};
	vk::ImageAspectFlagBits aspect{};
	vk::SampleCountFlagBits samples{};
	std::uint32_t mip_levels{};
};

struct ImageBarrier {
	vk::PipelineStageFlags2 src_stages{vk::PipelineStageFlagBits2::eAllCommands};
	vk::AccessFlags2 src_access{vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite};
	vk::PipelineStageFlags2 dst_stages{vk::PipelineStageFlagBits2::eAllCommands};
	vk::AccessFlags2 dst_access{vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite};
};

/// \brief Opaque interface representing a Vulkan Image.
class IRenderImage : public Polymorphic {
  public:
	using CreateInfo = ImageCreateInfo;

	[[nodiscard]] virtual auto get_image_info() const -> RenderImageInfo const& = 0;

	virtual auto resize(vk::Extent2D extent) -> bool = 0;
	virtual void overwrite(BitmapView bitmap, glm::ivec2 offset = {}) = 0;
	virtual void write_cube(std::span<BitmapView const, 6> layers) = 0;
	virtual void transition_layout(vk::CommandBuffer command_buffer, ImageBarrier const& barrier, vk::ImageLayout layout) = 0;
};

struct BufferCreateInfo {
	vk::BufferUsageFlags usage{};
	vk::DeviceSize size{};
	bool map_memory{true};
};

struct RenderBufferInfo {
	vk::Buffer buffer{};
	vk::BufferUsageFlags usage{};
	vk::DeviceSize capacity{};
};

struct BufferData {
	void const* data{};
	std::size_t size{};
};

/// \brief Opaque interface representing a Vulkan Buffer.
class IRenderBuffer : public Polymorphic {
  public:
	using CreateInfo = BufferCreateInfo;

	[[nodiscard]] virtual auto get_buffer_info() const -> RenderBufferInfo const& = 0;

	virtual void write_sequential(std::span<BufferData const> data) = 0;

	void write_data(BufferData const buffer_data) {
		if (buffer_data.data == nullptr || buffer_data.size == 0) { return; }
		write_sequential({&buffer_data, 1});
	}

	virtual void set_name(CString name) = 0;
};
} // namespace levk
