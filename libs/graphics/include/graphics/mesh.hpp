#pragma once
#include <core/ref.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/geometry.hpp>

namespace le::graphics {
class Device;
class CommandBuffer;

class Mesh {
  public:
	enum class Type { eStatic, eDynamic };
	struct Data {
		Ref<Buffer const> buffer;
		u32 count = 0;
	};

	inline static auto s_trisDrawn = std::atomic<u32>(0);

	Mesh(not_null<VRAM*> vram, Type type = Type::eStatic);
	Mesh(Mesh&&);
	Mesh& operator=(Mesh&&);
	virtual ~Mesh();

	template <typename T = glm::vec3>
	bool construct(View<T> vertices, View<u32> indices);
	template <VertType V>
	bool construct(Geom<V> const& geom);
	bool draw(CommandBuffer const& cb, u32 instances = 1, u32 first = 0) const;

	bool valid() const noexcept;
	bool busy() const;
	bool ready() const;
	void wait() const;

	Data vbo() const noexcept;
	Data ibo() const noexcept;
	Type type() const noexcept;

	bool hasIndices() const noexcept;

	not_null<VRAM*> m_vram;

  private:
	struct Storage {
		std::optional<Buffer> buffer;
		u32 count = 0;
		VRAM::Future transfer;
	};

	Storage construct(vk::BufferUsageFlags usage, void* pData, std::size_t size) const;
	void destroy();

	Storage m_vbo;
	Storage m_ibo;
	u32 m_triCount = 0;

	Type m_type;
};

// impl

template <typename T>
bool Mesh::construct(View<T> vertices, View<u32> indices) {
	if (!vertices.empty()) {
		destroy();
		m_vbo = construct(vk::BufferUsageFlagBits::eVertexBuffer, (void*)vertices.data(), vertices.size() * sizeof(T));
		if (!indices.empty()) {
			m_ibo = construct(vk::BufferUsageFlagBits::eIndexBuffer, (void*)indices.data(), indices.size() * sizeof(u32));
		}
		m_vbo.count = (u32)vertices.size();
		m_ibo.count = (u32)indices.size();
		m_triCount = indices.empty() ? u32(vertices.size() / 3) : u32(indices.size() / 3);
		return true;
	}
	return false;
}

template <VertType V>
bool Mesh::construct(Geom<V> const& geom) {
	return construct<Vert<V>>(geom.vertices, geom.indices);
}

inline Mesh::Data Mesh::vbo() const noexcept { return {*m_vbo.buffer, m_vbo.count}; }
inline Mesh::Data Mesh::ibo() const noexcept { return {*m_ibo.buffer, m_ibo.count}; }
inline Mesh::Type Mesh::type() const noexcept { return m_type; }
inline bool Mesh::hasIndices() const noexcept { return m_ibo.count > 0 && m_ibo.buffer && m_ibo.buffer->buffer() != vk::Buffer(); }
} // namespace le::graphics
