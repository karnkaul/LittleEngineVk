#include <graphics/context/command_buffer.hpp>
#include <graphics/context/device.hpp>
#include <graphics/mesh.hpp>

namespace le::graphics {
Mesh::Mesh(not_null<VRAM*> vram, Type type) : m_vram(vram), m_type(type) {}
Mesh::Mesh(Mesh&& rhs)
	: m_vram(rhs.m_vram), m_vbo(std::exchange(rhs.m_vbo, Storage())), m_ibo(std::exchange(rhs.m_ibo, Storage())), m_triCount(rhs.m_triCount),
	  m_type(rhs.m_type) {}
Mesh& Mesh::operator=(Mesh&& rhs) {
	if (&rhs != this) {
		destroy();
		m_vbo = std::exchange(rhs.m_vbo, Storage());
		m_ibo = std::exchange(rhs.m_ibo, Storage());
		m_triCount = std::exchange(rhs.m_triCount, 0);
		m_type = rhs.m_type;
	}
	return *this;
}
Mesh::~Mesh() { destroy(); }

Mesh::Storage Mesh::construct(vk::BufferUsageFlags usage, void* pData, std::size_t size) const {
	Storage ret;
	ret.buffer = m_vram->makeBuffer(size, usage, m_type == Type::eDynamic);
	ENSURE(ret.buffer, "Invalid buffer");
	if (m_type == Type::eStatic) {
		ret.transfer = m_vram->stage(*ret.buffer, pData, size);
	} else {
		[[maybe_unused]] bool const bRes = ret.buffer->write(pData, size);
		ENSURE(bRes, "Write failure");
	}
	return ret;
}

bool Mesh::draw(CommandBuffer const& cb, u32 instances, u32 first) const {
	if (valid()) {
		Buffer const* ibo = m_ibo.buffer.has_value() ? &*m_ibo.buffer : nullptr;
		cb.bindVBO(*m_vbo.buffer, ibo);
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

bool Mesh::valid() const noexcept { return m_vbo.buffer.has_value(); }

bool Mesh::busy() const {
	if (!valid() || m_type == Type::eDynamic) {
		return false;
	}
	return m_vbo.transfer.busy() || m_ibo.transfer.busy();
}

bool Mesh::ready() const { return valid() && m_vbo.transfer.ready(true) && m_ibo.transfer.ready(true); }

void Mesh::wait() const {
	if (m_type == Type::eDynamic) {
		using RF = Ref<VRAM::Future const>;
		std::array const arr = {RF(m_vbo.transfer), RF(m_ibo.transfer)};
		m_vram->wait(arr);
	}
}

void Mesh::destroy() {
	wait();
	m_vbo = {};
	m_ibo = {};
}
} // namespace le::graphics
