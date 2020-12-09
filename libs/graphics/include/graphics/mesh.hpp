#pragma once
#include <core/view.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/geometry.hpp>

namespace le::graphics {
class Device;

class Mesh {
  public:
	enum class Type { eStatic, eDynamic };
	struct Data {
		Ref<Buffer const> buffer;
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
	void wait() const;

	Data vbo() const noexcept;
	Data ibo() const noexcept;
	Type type() const noexcept;

	constexpr bool hasIndices() const noexcept;

	std::string m_name;
	Ref<VRAM> m_vram;

  protected:
	struct Storage {
		std::optional<Buffer> buffer;
		u32 count = 0;
		VRAM::Future transfer;
	};

	Storage construct(std::string_view name, vk::BufferUsageFlags usage, void* pData, std::size_t size) const;

	Storage m_vbo;
	Storage m_ibo;

  private:
	Type m_type;
};

// impl

template <typename T>
bool Mesh::construct(Span<T> vertices, Span<u32> indices) {
	destroy();
	if (!vertices.empty()) {
		m_vbo = construct(m_name + "/vbo", vk::BufferUsageFlagBits::eVertexBuffer, (void*)vertices.data(), vertices.size() * sizeof(T));
		if (!indices.empty()) {
			m_ibo = construct(m_name + "/ibo", vk::BufferUsageFlagBits::eIndexBuffer, (void*)indices.data(), indices.size() * sizeof(u32));
		}
		m_vbo.count = (u32)vertices.size();
		m_ibo.count = (u32)indices.size();
		return true;
	}
	return false;
}

template <VertType V>
bool Mesh::construct(Geom<V> const& geom) {
	return construct<Vert<V>>(geom.vertices, geom.indices);
}

inline Mesh::Data Mesh::vbo() const noexcept {
	return {*m_vbo.buffer, m_vbo.count};
}
inline Mesh::Data Mesh::ibo() const noexcept {
	return {*m_ibo.buffer, m_ibo.count};
}
inline Mesh::Type Mesh::type() const noexcept {
	return m_type;
}
inline constexpr bool Mesh::hasIndices() const noexcept {
	return m_ibo.count > 0 && m_ibo.buffer && m_ibo.buffer->buffer() != vk::Buffer();
}
} // namespace le::graphics
