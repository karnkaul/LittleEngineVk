#pragma once
#include <levk/core/not_null.hpp>
#include <levk/geometry.hpp>
#include <levk/material.hpp>
#include <levk/render_camera.hpp>
#include <levk/render_instance.hpp>
#include <levk/render_shader.hpp>
#include <levk/vertex_buffer.hpp>
#include <cstdint>

namespace levk {
/// \brief Abstract base for mesh primitives.
class IPrimitive : public Polymorphic {
  public:
	explicit IPrimitive(NotNull<IMaterial const*> material, RenderShader const& vertex_shader) : material(material), vertex_shader(vertex_shader) {}

	[[nodiscard]] virtual auto get_triangle_count() const -> std::int64_t = 0;

	[[nodiscard]] virtual auto get_topology() const -> vk::PrimitiveTopology = 0;
	[[nodiscard]] virtual auto get_vertex_binding() const -> VertexBinding = 0;

	[[nodiscard]] virtual auto get_vertex_array() const -> VertexArrayVbo = 0;
	[[nodiscard]] virtual auto get_vertex_skin() const -> VertexSkinVbo = 0;

	NotNull<IMaterial const*> material;
	RenderShader vertex_shader{};
};

/// \brief Opaque interface for a static mesh primitive.
class IStaticPrimitive : public IPrimitive {
  public:
	using IPrimitive::IPrimitive;

	[[nodiscard]] auto get_vertex_binding() const -> VertexBinding final { return VertexBinding::eDefault; }
};

/// \brief Opaque interface for a dynamic mesh primitive.
class IDynamicPrimitive : public IPrimitive {
  public:
	using IPrimitive::IPrimitive;

	[[nodiscard]] auto get_vertex_binding() const -> VertexBinding final { return VertexBinding::eDefault; }

	virtual void set_geometry(Geometry geometry) = 0;
};

/// \brief Opaque interface for a skinned mesh primitive.
class ISkinnedPrimitive : public IPrimitive {
  public:
	using IPrimitive::IPrimitive;

	[[nodiscard]] auto get_vertex_binding() const -> VertexBinding final { return VertexBinding::eSkinned; }
};

using PrimitivesView = std::span<NotNull<IPrimitive const*> const>;
} // namespace levk
