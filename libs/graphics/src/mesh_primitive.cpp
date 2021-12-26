#include <graphics/command_buffer.hpp>
#include <graphics/context/device.hpp>
#include <graphics/mesh_primitive.hpp>

namespace le::graphics {
MeshPrimitive::MeshPrimitive(not_null<VRAM*> vram, Type type) : m_vram(vram), m_type(type) {}

MeshPrimitive::~MeshPrimitive() { wait(); }

void MeshPrimitive::exchg(MeshPrimitive& lhs, MeshPrimitive& rhs) noexcept {
	std::swap(lhs.m_vbo, rhs.m_vbo);
	std::swap(lhs.m_ibo, rhs.m_ibo);
	std::swap(lhs.m_type, rhs.m_type);
	std::swap(lhs.m_triCount, rhs.m_triCount);
	std::swap(lhs.m_vram, rhs.m_vram);
}

MeshPrimitive::Storage MeshPrimitive::construct(vk::BufferUsageFlags usage, void* pData, std::size_t size) const {
	Storage ret;
	ret.buffer = m_vram->makeBuffer(size, usage, m_type == Type::eDynamic);
	ENSURE(ret.buffer.has_value(), "Invalid buffer");
	if (m_type == Type::eStatic) {
		ret.transfer = m_vram->stage(*ret.buffer, pData, size);
	} else {
		[[maybe_unused]] bool const bRes = ret.buffer->write(pData, size);
		ENSURE(bRes, "Write failure");
	}
	return ret;
}

bool MeshPrimitive::draw(CommandBuffer cb, u32 instances, u32 first) const {
	if (ready()) {
		cb.bindVBO(*m_vbo.buffer, m_ibo.buffer.has_value() ? &*m_ibo.buffer : nullptr);
		if (hasIndices()) {
			cb.drawIndexed(m_ibo.count, instances, first);
		} else {
			cb.draw(m_vbo.count, instances, first);
		}
		s_trisDrawn.fetch_add(m_triCount);
		return true;
	}
	return false;
}

bool MeshPrimitive::valid() const noexcept { return m_vbo.buffer.has_value(); }

bool MeshPrimitive::busy() const {
	if (!valid() || m_type == Type::eDynamic) { return false; }
	return m_vbo.transfer.busy() || m_ibo.transfer.busy();
}

bool MeshPrimitive::ready() const { return valid() && (m_type == Type::eDynamic || (m_vbo.transfer.ready() && m_ibo.transfer.ready())); }

void MeshPrimitive::wait() const {
	if (m_type == Type::eDynamic) {
		using RF = Ref<VRAM::Future const>;
		std::array const arr = {RF(m_vbo.transfer), RF(m_ibo.transfer)};
		m_vram->wait(arr);
	}
}
} // namespace le::graphics
