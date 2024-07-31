#pragma once
#include <levk/render_device.hpp>
#include <levk/vertex_buffer.hpp>

namespace levk {
struct Common {
	[[nodiscard]] static auto write_vertices(IRenderBuffer& buffer, VertexArray const& vertex_array) -> VertexArrayVbo {
		if (vertex_array.is_empty()) { return {}; }

		auto const vertices = std::span{vertex_array.vertices};
		auto const indices = std::span{vertex_array.indices};

		if (indices.empty()) {
			buffer.write_data({vertices.data(), vertices.size_bytes()});
		} else {
			auto const buffer_data = std::array{
				BufferData{vertices.data(), vertices.size_bytes()},
				BufferData{indices.data(), indices.size_bytes()},
			};
			buffer.write_sequential(buffer_data);
		}

		return VertexArrayVbo{
			.buffer = buffer.get_buffer_info().buffer,
			.vertex_count = static_cast<std::uint32_t>(vertices.size()),
			.index_count = static_cast<std::uint32_t>(indices.size()),
			.index_offset = vertices.size_bytes(),
		};
	}

	[[nodiscard]] static auto write_skin(IRenderBuffer& buffer, VertexSkin const& vertex_skin) -> VertexSkinVbo {
		auto const joint_indices = std::span{vertex_skin.joint_indices};
		auto const weights = std::span{vertex_skin.weights};

		auto const buffer_data = std::array{
			BufferData{joint_indices.data(), joint_indices.size_bytes()},
			BufferData{weights.data(), weights.size_bytes()},
		};
		buffer.write_sequential(buffer_data);

		return VertexSkinVbo{
			.buffer = buffer.get_buffer_info().buffer,
			.weights_offset = joint_indices.size_bytes(),
		};
	}
};

class StaticVertexBuffer : public IStaticVertexBuffer {
  public:
	explicit StaticVertexBuffer(NotNull<IRenderDevice*> device, VertexArray const& vertex_array) {
		if (vertex_array.is_empty()) { return; }

		auto const bci = BufferCreateInfo{
			.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
			.size = vertex_array.size_bytes(),
			.map_memory = false,
		};
		m_buffer = device->create_buffer(bci);
		m_verts = Common::write_vertices(*m_buffer, vertex_array);
		m_buffer->set_name("vertex vbo");
	}

	explicit StaticVertexBuffer(NotNull<IRenderDevice*> device, VertexSkin const& vertex_skin) {
		if (vertex_skin.is_empty()) { return; }

		auto const bci = BufferCreateInfo{
			.usage = vk::BufferUsageFlagBits::eVertexBuffer,
			.size = vertex_skin.size_bytes(),
			.map_memory = false,
		};
		m_buffer = device->create_buffer(bci);
		m_skin = Common::write_skin(*m_buffer, vertex_skin);
		m_buffer->set_name("skin vbo");
	}

  private:
	[[nodiscard]] auto get_vertices() const -> VertexArrayVbo const& final { return m_verts; }
	[[nodiscard]] auto get_skin() const -> VertexSkinVbo const& final { return m_skin; }

	std::unique_ptr<IRenderBuffer> m_buffer{};
	VertexArrayVbo m_verts{};
	VertexSkinVbo m_skin{};
};

class DynamicVertexBuffer : public IDynamicVertexBuffer {
  public:
	explicit DynamicVertexBuffer(NotNull<IRenderDevice*> device, VertexArray vertices) : m_device(device), m_vertices(std::move(vertices)) {
		auto const total_size = m_vertices.size_bytes();
		auto const bci = BufferCreateInfo{
			.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
			.size = total_size,
			.map_memory = true,
		};
		for (auto& storage : m_storage) {
			storage.buffer = device->create_buffer(bci);
			storage.buffer->set_name("dynamic vbo");
			storage.verts = Common::write_vertices(*storage.buffer, m_vertices);
		}
	}

  private:
	[[nodiscard]] auto get_vertices() const -> VertexArrayVbo const& final {
		auto& storage = m_storage.at(m_device->get_frame_index());
		if (storage.dirty) {
			storage.verts = Common::write_vertices(*storage.buffer, m_vertices);
			storage.dirty = false;
		}

		return storage.verts;
	}

	[[nodiscard]] auto get_skin() const -> VertexSkinVbo const& final {
		static constexpr auto blank_v = VertexSkinVbo{};
		return blank_v;
	}

	void set_vertices(VertexArray vertex_array) final {
		m_vertices = std::move(vertex_array);
		for (auto& storage : m_storage) { storage.dirty = true; }
	}

	struct Storage {
		std::unique_ptr<IRenderBuffer> buffer{};
		VertexArrayVbo verts{};
		bool dirty{};
	};

	NotNull<IRenderDevice*> m_device;

	mutable Buffered<Storage> m_storage{};
	VertexArray m_vertices{};
};
} // namespace levk
