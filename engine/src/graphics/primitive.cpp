#include <spaced/graphics/cache/vertex_buffer_cache.hpp>
#include <spaced/graphics/primitive.hpp>
#include <spaced/graphics/renderer.hpp>
#include <utility>

namespace spaced::graphics {
namespace {
auto write_vertices_indices(Buffer& out, Geometry const& geometry) -> vk::DeviceSize {
	auto const vertices = std::span{geometry.vertices};
	auto const indices = std::span{geometry.indices};
	auto const vibo_size = vertices.size_bytes() + indices.size_bytes();

	auto bytes = std::vector<std::uint8_t>(vibo_size);
	std::memcpy(bytes.data(), vertices.data(), vertices.size_bytes());
	if (!indices.empty()) {
		auto subspan = std::span{bytes}.subspan(vertices.size_bytes());
		std::memcpy(subspan.data(), indices.data(), indices.size_bytes());
	}

	out.write(bytes.data(), bytes.size());

	return vertices.size_bytes();
}

auto write_bones(std::unique_ptr<DeviceBuffer>& out, Geometry const& geometry) {
	auto const bones = std::span{geometry.bones};
	if (!bones.empty()) {
		if (!out) { out = std::make_unique<DeviceBuffer>(vk::BufferUsageFlagBits::eVertexBuffer, bones.size_bytes()); }
		out->write(bones.data(), bones.size_bytes());
	} else {
		out.reset();
	}
}
} // namespace

auto Primitive::set_geometry(Geometry&& geometry) -> void { set_geometry(std::as_const(geometry)); }

auto Primitive::draw(Buffers const& buffers, std::uint32_t const instances, vk::CommandBuffer const cmd) const -> void {
	auto const& bindings = PipelineCache::self().shader_layout().vertex_layout.buffers;
	assert(buffers.vertices);

	cmd.bindVertexBuffers(bindings.vertex.buffer, buffers.vertices, vk::DeviceSize{});
	if (buffers.indices != nullptr) { cmd.bindIndexBuffer(buffers.indices, buffers.index_offset, vk::IndexType::eUint32); }

	if (buffers.bones != nullptr) {
		cmd.bindVertexBuffers(bindings.skeleton.buffer, buffers.bones, vk::DeviceSize{});
	} else {
		auto const& empty_buffer = ScratchBufferCache::self().get_empty_buffer(vk::BufferUsageFlagBits::eVertexBuffer);
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
	auto const vibo_size = std::span{geometry.vertices}.size_bytes() + std::span{geometry.indices}.size_bytes();

	if (!m_data.get().vertices_indices || m_data.get().vertices_indices->size() < vibo_size) {
		m_data.get().vertices_indices =
			std::make_unique<DeviceBuffer>(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer, vibo_size);
	}

	m_data.get().index_offset = write_vertices_indices(*m_data.get().vertices_indices, geometry);
	write_bones(m_data.get().bones, geometry);

	m_layout.vertex_count = static_cast<std::uint32_t>(geometry.vertices.size());
	m_layout.index_count = static_cast<std::uint32_t>(geometry.indices.size());
	m_layout.bone_count = static_cast<std::uint32_t>(geometry.bones.size());
}

auto StaticPrimitive::draw(std::uint32_t const instances, vk::CommandBuffer const cmd) const -> void {
	if (!m_data.get().vertices_indices) { return; }
	auto const buffers = Buffers{
		.vertices = m_data.get().vertices_indices->buffer(),
		.indices = m_data.get().index_offset > 0 ? m_data.get().vertices_indices->buffer() : vk::Buffer{},
		.bones = m_data.get().bones ? m_data.get().bones->buffer() : vk::Buffer{},
		.index_offset = m_data.get().index_offset,
	};
	Primitive::draw(buffers, instances, cmd);
}

DynamicPrimitive::DynamicPrimitive() { m_vertices_indices = VertexBufferCache::self().allocate(); }

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
		.vertices = m_vertices_indices[index].get()->buffer(),
		.indices = m_vertices_indices[index].get()->buffer(),
		.index_offset = m_index_offset,
	};
	Primitive::draw(buffers, instances, cmd);
}

auto DynamicPrimitive::write_at(FrameIndex index) const -> void {
	assert(m_vertices_indices[index] != nullptr);

	m_index_offset = write_vertices_indices(*m_vertices_indices[index], m_geometry);
}
} // namespace spaced::graphics
