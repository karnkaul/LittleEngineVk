#pragma once
#include <levk/primitive.hpp>
#include <levk/render_device.hpp>
#include <memory>

namespace levk {
struct Primitive {
	explicit Primitive(NotNull<IRenderDevice*> device) : m_device(device) {}

	[[nodiscard]] auto do_get_triangle_count() const -> std::int64_t {
		auto const vbo = do_get_vertex_array();
		if (vbo.vertex_count == 0) { return 0; }
		auto const vertices = vbo.index_count == 0 ? vbo.vertex_count : vbo.index_count;
		return compute_triangle_count(vertices, m_topology);
	}

	[[nodiscard]] auto do_get_vertex_array() const -> VertexArrayVbo { return m_vertex_array ? m_vertex_array->get_vertices() : VertexArrayVbo{}; }
	[[nodiscard]] auto do_get_vertex_skin() const -> VertexSkinVbo { return m_vertex_skin ? m_vertex_skin->get_skin() : VertexSkinVbo{}; }

  protected:
	std::unique_ptr<IVertexBuffer> m_vertex_array{};
	std::unique_ptr<IVertexBuffer> m_vertex_skin{};
	vk::PrimitiveTopology m_topology{};

	NotNull<IRenderDevice*> m_device;

	mutable std::vector<RenderInstance::Baked> m_baked_instances{};
};

class StaticPrimitive : public IStaticPrimitive, Primitive {
  public:
	explicit StaticPrimitive(NotNull<IRenderDevice*> device, Geometry const& geometry, NotNull<IMaterial const*> material, RenderShader vertex_shader)
		: IStaticPrimitive(material, vertex_shader), Primitive(device) {
		m_topology = geometry.topology;
		m_vertex_array = device->create_static_vertex_buffer(geometry.vertex_array);
	}

  private:
	[[nodiscard]] auto get_triangle_count() const -> std::int64_t final { return do_get_triangle_count(); }
	[[nodiscard]] auto get_topology() const -> vk::PrimitiveTopology final { return m_topology; }
	[[nodiscard]] auto get_vertex_array() const -> VertexArrayVbo final { return do_get_vertex_array(); }
	[[nodiscard]] auto get_vertex_skin() const -> VertexSkinVbo final { return do_get_vertex_skin(); }
};

class DynamicPrimitive : public IDynamicPrimitive, Primitive {
  public:
	explicit DynamicPrimitive(NotNull<IRenderDevice*> device, Geometry geometry, NotNull<IMaterial const*> material, RenderShader vertex_shader)
		: IDynamicPrimitive(material, vertex_shader), Primitive(device) {
		set_geometry(std::move(geometry));
	}

  private:
	[[nodiscard]] auto get_triangle_count() const -> std::int64_t final { return do_get_triangle_count(); }
	[[nodiscard]] auto get_topology() const -> vk::PrimitiveTopology final { return m_topology; }
	[[nodiscard]] auto get_vertex_array() const -> VertexArrayVbo final { return do_get_vertex_array(); }
	[[nodiscard]] auto get_vertex_skin() const -> VertexSkinVbo final { return do_get_vertex_skin(); }

	void set_geometry(Geometry geometry) final {
		m_topology = geometry.topology;
		if (!m_vertex_array) {
			m_vertex_array = m_device->create_dynamic_vertex_buffer(std::move(geometry.vertex_array));
		} else {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
			static_cast<IDynamicVertexBuffer&>(*m_vertex_array).set_vertices(std::move(geometry.vertex_array));
		}
	}
};

class SkinnedPrimitive : public ISkinnedPrimitive, Primitive {
  public:
	explicit SkinnedPrimitive(NotNull<IRenderDevice*> device, Geometry const& geometry, NotNull<IMaterial const*> material, RenderShader vertex_shader)
		: ISkinnedPrimitive(material, vertex_shader), Primitive(device) {
		m_topology = geometry.topology;
		m_vertex_array = device->create_static_vertex_buffer(geometry.vertex_array);
		if (!geometry.vertex_skin.is_empty()) { m_vertex_skin = device->create_skin_vertex_buffer(geometry.vertex_skin); }
	}

  private:
	[[nodiscard]] auto get_triangle_count() const -> std::int64_t final { return do_get_triangle_count(); }
	[[nodiscard]] auto get_topology() const -> vk::PrimitiveTopology final { return m_topology; }
	[[nodiscard]] auto get_vertex_array() const -> VertexArrayVbo final { return do_get_vertex_array(); }
	[[nodiscard]] auto get_vertex_skin() const -> VertexSkinVbo final { return do_get_vertex_skin(); }
};
} // namespace levk
