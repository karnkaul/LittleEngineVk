#pragma once
#include <le/core/ptr.hpp>
#include <le/graphics/buffering.hpp>
#include <le/graphics/defer.hpp>
#include <le/graphics/geometry.hpp>
#include <le/graphics/resource.hpp>
#include <optional>

namespace le::graphics {
class Primitive {
  public:
	Primitive(Primitive const&) = delete;
	auto operator=(Primitive const&) -> Primitive& = delete;

	Primitive(Primitive&&) = default;
	auto operator=(Primitive&&) -> Primitive& = default;

	Primitive() = default;
	virtual ~Primitive() = default;

	struct Layout {
		std::uint32_t vertex_count{};
		std::uint32_t index_count{};
		std::uint32_t bone_count{};
	};

	[[nodiscard]] auto layout() const -> Layout const& { return m_layout; }

	virtual auto set_geometry(Geometry const& geometry) -> void = 0;
	virtual auto set_geometry(Geometry&& geometry) -> void;
	virtual auto draw(std::uint32_t instances, vk::CommandBuffer cmd) const -> void = 0;

  protected:
	struct Buffers {
		vk::Buffer vertices{};
		vk::Buffer indices{};
		vk::Buffer bones{};
		std::optional<vk::DeviceSize> index_offset{};
	};

	auto draw(Buffers const& buffers, std::uint32_t instances, vk::CommandBuffer cmd) const -> void;

	Layout m_layout{};
};

class StaticPrimitive : public Primitive {
  public:
	auto set_geometry(Geometry const& geometry) -> void final;
	auto draw(std::uint32_t instances, vk::CommandBuffer cmd) const -> void final;

  protected:
	struct Data {
		std::unique_ptr<DeviceBuffer> vertices_indices{};
		std::unique_ptr<DeviceBuffer> bones{};
		std::optional<vk::DeviceSize> index_offset{};
	};

	Defer<Data> m_data{};
};

class DynamicPrimitive : public Primitive {
  public:
	DynamicPrimitive();

	auto set_geometry(Geometry const& geometry) -> void final;
	auto set_geometry(Geometry&& geometry) -> void final;
	auto draw(std::uint32_t instances, vk::CommandBuffer cmd) const -> void final;

  protected:
	auto write_at(FrameIndex index) const -> void;

	Geometry m_geometry{};
	Buffered<std::shared_ptr<HostBuffer>> m_vertices_indices{};
	mutable std::optional<vk::DeviceSize> m_index_offset{};
};
} // namespace le::graphics
