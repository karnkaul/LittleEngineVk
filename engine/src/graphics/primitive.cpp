#include <spaced/graphics/primitive.hpp>
#include <spaced/graphics/renderer.hpp>
#include <utility>

namespace spaced::graphics {
auto Primitive::set_geometry(Geometry&& geometry) -> void { set_geometry(std::as_const(geometry)); }

auto Primitive::draw(Buffers const& buffers, std::uint32_t const instances, vk::CommandBuffer const cmd) const -> void {
	auto const& bindings = PipelineCache::self().shader_layout().vertex_layout.buffers;
	assert(buffers.geometry);

	cmd.bindVertexBuffers(bindings.geometry.buffer, buffers.geometry->buffer(), vk::DeviceSize{});
	if (buffers.indices != nullptr) { cmd.bindIndexBuffer(buffers.indices->buffer(), {}, vk::IndexType::eUint32); }

	if (buffers.bones != nullptr) {
		cmd.bindVertexBuffers(bindings.skeleton.buffer, buffers.bones->buffer(), vk::DeviceSize{});
	} else {
		auto const& empty_buffer = ScratchBufferCache::self().empty_buffer(vk::BufferUsageFlagBits::eVertexBuffer);
		cmd.bindVertexBuffers(bindings.skeleton.buffer, empty_buffer.buffer(), vk::DeviceSize{});
	}

	if (m_layout.index_count > 0) {
		assert(buffers.indices);
		cmd.drawIndexed(m_layout.index_count, instances, 0, 0, 0);
	} else {
		cmd.draw(m_layout.vertex_count, instances, 0, 0);
	}
}

auto StaticPrimitive::set_geometry(Geometry const& geometry) -> void {
	auto const vertices = std::span{geometry.vertices};
	if (!m_data.get().vertices) { m_data.get().vertices = std::make_unique<DeviceBuffer>(vk::BufferUsageFlagBits::eVertexBuffer, vertices.size_bytes()); }
	m_data.get().vertices->write(vertices.data(), vertices.size_bytes());

	auto const indices = std::span{geometry.indices};
	if (!indices.empty()) {
		if (!m_data.get().indices) { m_data.get().indices = std::make_unique<DeviceBuffer>(vk::BufferUsageFlagBits::eIndexBuffer, indices.size_bytes()); }
		m_data.get().indices->write(indices.data(), indices.size_bytes());
	}

	auto const bones = std::span{geometry.bones};
	if (!bones.empty()) {
		if (!m_data.get().bones) { m_data.get().bones = std::make_unique<DeviceBuffer>(vk::BufferUsageFlagBits::eVertexBuffer, bones.size_bytes()); }
		m_data.get().bones->write(bones.data(), bones.size_bytes());
	} else {
		m_data.get().bones.reset();
	}

	m_layout.vertex_count = static_cast<std::uint32_t>(vertices.size());
	m_layout.index_count = static_cast<std::uint32_t>(indices.size());
	m_layout.bone_count = static_cast<std::uint32_t>(bones.size());
}

auto StaticPrimitive::draw(std::uint32_t const instances, vk::CommandBuffer const cmd) const -> void {
	if (!m_data.get().vertices) { return; }
	auto const buffers = Buffers{
		.geometry = m_data.get().vertices.get(),
		.indices = m_data.get().indices.get(),
		.bones = m_data.get().bones.get(),
	};
	Primitive::draw(buffers, instances, cmd);
}

DynamicPrimitive::DynamicPrimitive() {
	fill_buffered(m_data.get().vertices, [] { return std::make_unique<HostBuffer>(vk::BufferUsageFlagBits::eVertexBuffer); });
	fill_buffered(m_data.get().indices, [] { return std::make_unique<HostBuffer>(vk::BufferUsageFlagBits::eIndexBuffer); });
}

auto DynamicPrimitive::set_geometry(Geometry const& geometry) -> void {
	auto copy = geometry;
	set_geometry(std::move(copy));
}

auto DynamicPrimitive::set_geometry(Geometry&& geometry) -> void {
	m_geometry = std::move(geometry);

	m_layout.vertex_count = static_cast<std::uint32_t>(m_geometry.vertices.size());
	m_layout.index_count = static_cast<std::uint32_t>(m_geometry.indices.size());
}

auto DynamicPrimitive::draw(std::uint32_t instances, vk::CommandBuffer cmd) const -> void {
	if (m_geometry.vertices.empty()) { return; }

	auto const index = Renderer::self().get_frame_index();
	write_at(index);

	auto const buffers = Buffers{
		.geometry = m_data.get().vertices[index].get(),
		.indices = m_data.get().indices[index].get(),
	};
	Primitive::draw(buffers, instances, cmd);
}

auto DynamicPrimitive::write_at(FrameIndex index) const -> void {
	assert(m_data.get().vertices[index] != nullptr && m_data.get().indices[index] != nullptr);

	auto const vertices = std::span{m_geometry.vertices};
	m_data.get().vertices[index]->write(vertices.data(), vertices.size_bytes());

	auto const indices = std::span{m_geometry.indices};
	if (!indices.empty()) { m_data.get().indices[index]->write(indices.data(), indices.size_bytes()); }
}
} // namespace spaced::graphics
