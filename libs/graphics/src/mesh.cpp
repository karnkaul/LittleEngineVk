#include <core/utils.hpp>
#include <graphics/context/device.hpp>
#include <graphics/mesh.hpp>

namespace le::graphics {
Mesh::Mesh(std::string name, VRAM& vram, Type type) : m_name(std::move(name)), m_vram(vram), m_type(type) {
}
Mesh::Mesh(Mesh&& rhs)
	: m_name(std::move(rhs.m_name)), m_vbo(std::exchange(rhs.m_vbo, Storage())), m_ibo(std::exchange(rhs.m_ibo, Storage())), m_vram(rhs.m_vram),
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
	VRAM& v = m_vram;
	Storage ret;
	ret.data.buffer = v.createBO(name, size, usage, m_type == Type::eDynamic);
	if (m_type == Type::eStatic) {
		ret.transfer = v.stage(ret.data.buffer, pData, size);
	} else {
		[[maybe_unused]] bool const bRes = v.write(ret.data.buffer, pData, {0, size});
		ENSURE(bRes, "Write failure");
	}
	return ret;
}

void Mesh::destroy() {
	VRAM& v = m_vram;
	Device& d = v.m_device;
	d.defer([&v, vb = m_vbo.data, ib = m_ibo.data]() mutable {
		v.destroy(vb.buffer);
		v.destroy(ib.buffer);
	});
	m_vbo = {};
	m_ibo = {};
}

bool Mesh::valid() const {
	return m_vbo.data.buffer.valid();
}

bool Mesh::busy() const {
	if (m_type == Type::eStatic) {
		return false;
	}
	return !utils::ready(m_vbo.transfer) || !utils::ready(m_ibo.transfer);
}

bool Mesh::ready() const {
	return valid() && !busy();
}

void Mesh::wait() {
	if (m_type == Type::eDynamic) {
		VRAM& v = m_vram;
		v.wait({m_vbo.transfer, m_ibo.transfer});
	}
}
} // namespace le::graphics
