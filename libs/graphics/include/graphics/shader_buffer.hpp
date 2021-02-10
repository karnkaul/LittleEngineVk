#pragma once
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/descriptor_set.hpp>
#include <graphics/resources.hpp>
#include <graphics/utils/ring_buffer.hpp>

namespace le::graphics {
struct ShaderBufInfo {
	vk::DescriptorType type = vk::DescriptorType::eUniformBuffer;
	u32 rotateCount = 2;
};

namespace detail {
template <typename T, bool IsArray>
struct ShaderBufTraits;
} // namespace detail

template <typename T, bool IsArray>
class ShaderBuffer;

template <typename T, bool IsArray>
using TBuf = ShaderBuffer<T, IsArray>;

template <typename T, bool IsArray>
class ShaderBuffer {
  public:
	using traits = detail::ShaderBufTraits<T, IsArray>;
	using type = typename traits::type;
	using value_type = typename traits::value_type;

	static_assert(std::is_trivial_v<value_type>, "value_type must be trivial");

	static constexpr std::size_t bufSize = sizeof(value_type);
	static constexpr vk::BufferUsageFlagBits usage(vk::DescriptorType type) noexcept;

	ShaderBuffer(VRAM& vram, std::string_view name, ShaderBufInfo const& info);

	void set(type const& t);
	void set(type&& t);
	type const& get() const;
	type& get();
	void write(std::optional<T> t = std::nullopt);
	void update(DescriptorSet& out_set, u32 binding) const;
	void swap();

  private:
	void resize(std::size_t size);

	struct Storage {
		std::vector<RingBuffer<Buffer>> buffers;
		type t;
		std::string name;
		vk::BufferUsageFlagBits usage = {};
		u32 rotateCount = 0;
	};

	Storage m_storage;
	Ref<VRAM> m_vram;
};

// impl

template <typename T, bool IsArray>
constexpr vk::BufferUsageFlagBits ShaderBuffer<T, IsArray>::usage(vk::DescriptorType type) noexcept {
	switch (type) {
	case vk::DescriptorType::eStorageBuffer:
		return vk::BufferUsageFlagBits::eStorageBuffer;
	default:
		return vk::BufferUsageFlagBits::eUniformBuffer;
	}
}

template <typename T, bool IsArray>
ShaderBuffer<T, IsArray>::ShaderBuffer(VRAM& vram, std::string_view name, ShaderBufInfo const& info) : m_vram(vram) {
	m_storage.name = std::string(name);
	m_storage.usage = usage(info.type);
	m_storage.rotateCount = info.rotateCount;
}
template <typename T, bool IsArray>
void ShaderBuffer<T, IsArray>::set(type const& t) {
	m_storage.t = t;
	write();
}
template <typename T, bool IsArray>
void ShaderBuffer<T, IsArray>::set(type&& t) {
	m_storage.t = std::move(t);
	write();
}
template <typename T, bool IsArray>
typename ShaderBuffer<T, IsArray>::type const& ShaderBuffer<T, IsArray>::get() const {
	return m_storage.t;
}
template <typename T, bool IsArray>
typename ShaderBuffer<T, IsArray>::type& ShaderBuffer<T, IsArray>::get() {
	return m_storage.t;
}
template <typename T, bool IsArray>
void ShaderBuffer<T, IsArray>::write(std::optional<T> t) {
	if (t) {
		m_storage.t = std::move(*t);
	}
	if constexpr (IsArray) {
		resize(m_storage.t.size());
		std::size_t idx = 0;
		for (auto const& t : m_storage.t) {
			m_storage.buffers[idx++].get().write(&t, bufSize);
		}
	} else {
		resize(1);
		m_storage.buffers.front().get().write(&m_storage.t, bufSize);
	}
}
template <typename T, bool IsArray>
void ShaderBuffer<T, IsArray>::update(DescriptorSet& out_set, u32 binding) const {
	if constexpr (IsArray) {
		ENSURE(!m_storage.t.empty() && m_storage.t.size() == m_storage.buffers.size(), "Empty buffer!");
		std::vector<Ref<Buffer const>> vec;
		vec.reserve(m_storage.t.size());
		for (std::size_t idx = 0; idx < m_storage.t.size(); ++idx) {
			RingBuffer<Buffer> const& rb = m_storage.buffers[idx];
			vec.push_back(rb.get());
		}
		out_set.updateBuffers(binding, vec, bufSize);
	} else {
		ENSURE(!m_storage.buffers.empty(), "Empty buffer!");
		out_set.updateBuffers(binding, Ref<Buffer const>(m_storage.buffers.front().get()), bufSize);
	}
}
template <typename T, bool IsArray>
void ShaderBuffer<T, IsArray>::swap() {
	for (auto& rb : m_storage.buffers) {
		rb.next();
	}
}
template <typename T, bool IsArray>
void ShaderBuffer<T, IsArray>::resize(std::size_t size) {
	m_storage.buffers.reserve(size);
	for (std::size_t i = m_storage.buffers.size(); i < size; ++i) {
		RingBuffer<Buffer> buffer;
		io::Path prefix(m_storage.name);
		if constexpr (IsArray) {
			prefix += "[";
			prefix += std::to_string(i);
			prefix += "]";
		}
		for (u32 j = 0; j < m_storage.rotateCount; ++j) {
			buffer.ts.push_back(m_vram.get().createBO((prefix / std::to_string(j)).generic_string(), bufSize, m_storage.usage, true));
		}
		m_storage.buffers.push_back(std::move(buffer));
	}
}

namespace detail {
template <typename T, bool IsArray>
struct ShaderBufTraits;

template <typename T>
struct ShaderBufTraits<T, true> {
	using type = T;
	using value_type = typename T::value_type;
};
template <typename T>
struct ShaderBufTraits<T, false> {
	using type = T;
	using value_type = T;
};
} // namespace detail
} // namespace le::graphics
