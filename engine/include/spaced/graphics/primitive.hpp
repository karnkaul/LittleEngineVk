#pragma once
#include <spaced/core/ptr.hpp>
#include <spaced/graphics/buffering.hpp>
#include <spaced/graphics/defer.hpp>
#include <spaced/graphics/geometry.hpp>
#include <spaced/graphics/resource.hpp>

namespace spaced::graphics {
class Primitive {
  public:
	Primitive() = default;

	Primitive(Primitive const&) = delete;
	Primitive(Primitive&&) = delete;
	auto operator=(Primitive const&) -> Primitive& = delete;
	auto operator=(Primitive&&) -> Primitive& = delete;

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
		Ptr<Buffer const> geometry{};
		Ptr<Buffer const> indices{};
		Ptr<Buffer const> bones{};
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
		std::unique_ptr<DeviceBuffer> vertices{};
		std::unique_ptr<DeviceBuffer> indices{};
		std::unique_ptr<DeviceBuffer> bones{};
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
	struct Data {
		Buffered<std::unique_ptr<Buffer>> vertices{};
		Buffered<std::unique_ptr<Buffer>> indices{};
	};

	auto write_at(FrameIndex index) const -> void;

	Geometry m_geometry{};
	Defer<Data> m_data{};
};
} // namespace spaced::graphics
