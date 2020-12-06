#pragma once
#include <core/view.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/geometry.hpp>
#include <graphics/types.hpp>

namespace le::graphics {
class Device;

class Mesh {
  public:
	enum class Type { eStatic, eDynamic };
	struct Data {
		View<Buffer> buffer;
		u32 count = 0;
	};

	Mesh(std::string name, VRAM& vram, Type type = Type::eStatic);
	Mesh(Mesh&&);
	Mesh& operator=(Mesh&&);
	virtual ~Mesh();

	template <typename T = glm::vec3>
	bool construct(Span<T> vertices, Span<u32> indices);
	template <VertType V>
	bool construct(Geom<V> const& geom);
	void destroy();
	bool valid() const;
	bool busy() const;
	bool ready() const;
	void wait();

	Data vbo() const noexcept;
	Data ibo() const noexcept;
	Type type() const noexcept;

	constexpr bool hasIndices() const noexcept;

	std::string m_name;

  protected:
	struct Storage {
		Data data;
		VRAM::Future transfer;
	};

	Storage construct(std::string_view name, vk::BufferUsageFlags usage, void* pData, std::size_t size) const;

	Storage m_vbo;
	Storage m_ibo;

  private:
	Ref<VRAM> m_vram;
	Type m_type;
};

// impl

template <typename T>
bool Mesh::construct(Span<T> vertices, Span<u32> indices) {
	destroy();
	if (!vertices.empty()) {
		m_vbo = construct(m_name + "_vbo", vk::BufferUsageFlagBits::eVertexBuffer, (void*)vertices.data(), vertices.size() * sizeof(T));
		if (!indices.empty()) {
			m_ibo = construct(m_name + "_ibo", vk::BufferUsageFlagBits::eIndexBuffer, (void*)indices.data(), indices.size() * sizeof(u32));
		}
		m_vbo.data.count = (u32)vertices.size();
		m_ibo.data.count = (u32)indices.size();
		return true;
	}
	return false;
}

template <VertType V>
bool Mesh::construct(Geom<V> const& geom) {
	return construct<Vert<V>>(geom.vertices, geom.indices);
}

inline Mesh::Data Mesh::vbo() const noexcept {
	return m_vbo.data;
}
inline Mesh::Data Mesh::ibo() const noexcept {
	return m_ibo.data;
}
inline Mesh::Type Mesh::type() const noexcept {
	return m_type;
}
inline constexpr bool Mesh::hasIndices() const noexcept {
	return m_ibo.data.count > 0 && m_ibo.data.buffer.valid() && !default_v(m_ibo.data.buffer->buffer);
}
} // namespace le::graphics
