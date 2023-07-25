#pragma once
#include <vk_mem_alloc.h>
#include <glm/vec2.hpp>
#include <spaced/engine/graphics/common.hpp>
#include <span>

namespace spaced::graphics {
class Resource {
  public:
	Resource(Resource&&) = delete;
	Resource(Resource const&) = delete;
	auto operator=(Resource&&) -> Resource& = delete;
	auto operator=(Resource const&) -> Resource& = delete;

	virtual ~Resource() = default;

	[[nodiscard]] auto allocation() const -> VmaAllocation { return m_allocation; }

  protected:
	Resource() = default;

	VmaAllocation m_allocation{};
};

class HostBuffer;

class Buffer : public Resource {
  public:
	Buffer(Buffer&&) = delete;
	Buffer(Buffer const&) = delete;
	auto operator=(Buffer&&) -> Buffer& = delete;
	auto operator=(Buffer const&) -> Buffer& = delete;

	explicit Buffer(vk::BufferUsageFlags usage, vk::DeviceSize capacity = 1) : Buffer(usage, capacity, false) {}
	~Buffer() override;

	[[nodiscard]] auto usage() const -> vk::BufferUsageFlags { return m_usage; }
	[[nodiscard]] auto buffer() const -> vk::Buffer { return m_buffer; }
	[[nodiscard]] auto capacity() const -> vk::DeviceSize { return m_capacity; }
	[[nodiscard]] auto size() const -> vk::DeviceSize { return m_size; }

	auto resize(std::size_t new_capacity) -> void;

	virtual auto write(void const* data, std::size_t size) -> void = 0;

	operator vk::Buffer() const { return buffer(); }

  protected:
	explicit Buffer(vk::BufferUsageFlags usage, vk::DeviceSize capacity, bool host_visible);

	vk::BufferUsageFlags m_usage{};
	vk::DeviceSize m_capacity{};
	vk::Buffer m_buffer{};
	std::size_t m_size{};
	void* m_mapped{};
	bool m_host{};
};

class HostBuffer : public Buffer {
  public:
	explicit HostBuffer(vk::BufferUsageFlags usage, vk::DeviceSize capacity = 1) : Buffer(usage, capacity, true) {}

	[[nodiscard]] auto mapped() const -> void const* { return m_mapped; }
	[[nodiscard]] auto mapped() -> void* { return m_mapped; }

	auto write(void const* data, std::size_t size) -> void final;
};

class DeviceBuffer : public Buffer {
  public:
	explicit DeviceBuffer(vk::BufferUsageFlags usage, vk::DeviceSize capacity) : Buffer(usage, capacity, false) {}

	auto write(void const* data, std::size_t size) -> void final;
};

struct ImageCreateInfo {
	static constexpr auto usage_v = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

	vk::Format format{vk::Format::eR8G8B8A8Srgb};
	vk::ImageUsageFlags usage{usage_v};
	vk::ImageAspectFlagBits aspect{vk::ImageAspectFlagBits::eColor};
	vk::ImageTiling tiling{vk::ImageTiling::eOptimal};
	vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
	vk::ImageViewType view_type{vk::ImageViewType::e2D};
	bool mip_map{true};
};

class Image : Resource {
  public:
	using Layer = std::span<std::uint8_t const>;

	static constexpr vk::Extent2D min_extent_v{1, 1};
	static constexpr std::uint32_t cubemap_layers_v{6};

	static auto compute_mip_levels(vk::Extent2D extent) -> std::uint32_t;

	Image(Image&&) = delete;
	Image(Image const&) = delete;
	auto operator=(Image&&) -> Image& = delete;
	auto operator=(Image const&) -> Image& = delete;

	explicit Image(ImageCreateInfo const& create_info, vk::Extent2D extent = min_extent_v);

	~Image() override;

	auto resize(vk::Extent2D extent) -> void;
	auto copy_from(std::span<Layer const> layers, vk::Extent2D target_extent, vk::Offset2D offset) -> bool;

	[[nodiscard]] auto image() const -> vk::Image { return m_image; }
	[[nodiscard]] auto extent() const -> vk::Extent2D { return m_extent; }
	[[nodiscard]] auto format() const -> vk::Format { return m_create_info.format; }
	[[nodiscard]] auto image_view() const -> vk::ImageView { return *m_view; }
	[[nodiscard]] auto view_type() const -> vk::ImageViewType { return m_create_info.view_type; }
	[[nodiscard]] auto layout() const -> vk::ImageLayout { return m_layout; }
	[[nodiscard]] auto mip_levels() const -> std::uint32_t { return m_mip_levels; }

	operator vk::Image() const { return m_image; }
	operator vk::ImageView() const { return *m_view; }

  protected:
	ImageCreateInfo m_create_info{};
	vk::Image m_image{};
	vk::Extent2D m_extent{};
	vk::UniqueImageView m_view{};
	vk::ImageLayout m_layout{};
	std::uint32_t m_mip_levels{};
};
} // namespace spaced::graphics
