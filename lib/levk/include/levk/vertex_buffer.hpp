#pragma once
#include <levk/core/polymorphic.hpp>
#include <levk/geometry.hpp>
#include <vulkan/vulkan.hpp>

namespace levk {
struct VertexArrayVbo {
	vk::Buffer buffer{};
	std::uint32_t vertex_count{};
	std::uint32_t index_count{};
	vk::DeviceSize index_offset{};
};

struct VertexSkinVbo {
	vk::Buffer buffer{};
	vk::DeviceSize weights_offset{};
};

/// \brief Base interface for vertex buffers.
class IVertexBuffer : public Polymorphic {
  public:
	[[nodiscard]] virtual auto get_vertices() const -> VertexArrayVbo const& = 0;
	[[nodiscard]] virtual auto get_skin() const -> VertexSkinVbo const& = 0;
};

/// \brief Vertex buffer with static Geometry.
/// Uploads Geometry to GPU memory.
class IStaticVertexBuffer : public IVertexBuffer {};

/// \brief Vertex buffer with dynamic Geometry.
/// Keeps Geometry in buffered shared memory.
/// Allows overwriting stored Geometry anytime.
class IDynamicVertexBuffer : public IVertexBuffer {
  public:
	virtual void set_vertices(VertexArray vertex_array) = 0;
};
} // namespace levk
