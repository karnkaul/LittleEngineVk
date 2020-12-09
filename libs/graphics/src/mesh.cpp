#include <core/utils.hpp>
#include <graphics/context/device.hpp>
#include <graphics/mesh.hpp>

namespace le::graphics {
Mesh::Mesh(std::string name, VRAM& vram, Type type) : m_name(std::move(name)), m_vram(vram), m_type(type) {
}
Mesh::Mesh(Mesh&& rhs)
	: m_name(std::move(rhs.m_name)), m_vram(rhs.m_vram), m_vbo(std::exchange(rhs.m_vbo, Storage())), m_ibo(std::exchange(rhs.m_ibo, Storage())),
	  m_type(rhs.m_type) {
}
Mesh& Mesh::operator=(Mesh&& rhs) {
	if (&rhs != this) {
		destroy();
		m_name = std::move(rhs.m_name);
		m_vbo = std::exchange(rhs.m_vbo, Storage());
		m_ibo = std::exchange(rhs.m_ibo, Storage());
		m_type = rhs.m_type;
	}
	return *this;
}
Mesh::~Mesh() {
	destroy();
}

Mesh::Storage Mesh::construct(std::string_view name, vk::BufferUsageFlags usage, void* pData, std::size_t size) const {
	Storage ret;
	ret.buffer = m_vram.get().createBO(name, size, usage, m_type == Type::eDynamic);
	ENSURE(ret.buffer, "Invalid buffer");
	if (m_type == Type::eStatic) {
		ret.transfer = m_vram.get().stage(*ret.buffer, pData, size);
	} else {
		[[maybe_unused]] bool const bRes = ret.buffer->write(pData, size);
		ENSURE(bRes, "Write failure");
	}
	return ret;
}

void Mesh::destroy() {
	wait();
	m_vbo = {};
	m_ibo = {};
}

bool Mesh::valid() const {
	return m_vbo.buffer.has_value();
}

bool Mesh::busy() const {
	if (!valid() || m_type == Type::eDynamic) {
		return false;
	}
	return m_vbo.transfer.busy() || m_ibo.transfer.busy();
}

bool Mesh::ready() const {
	return valid() && m_vbo.transfer.ready(true) && m_ibo.transfer.ready(true);
}

void Mesh::wait() const {
	if (m_type == Type::eDynamic) {
		m_vram.get().wait({m_vbo.transfer, m_ibo.transfer});
	}
}
} // namespace le::graphics
