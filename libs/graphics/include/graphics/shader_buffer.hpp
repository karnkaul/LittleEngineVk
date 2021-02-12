#pragma once
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/resources.hpp>
#include <graphics/utils/ring_buffer.hpp>

namespace le::graphics {
class DescriptorSet;

class ShaderBuffer {
  public:
	struct CreateInfo;

	static constexpr vk::BufferUsageFlagBits usage(vk::DescriptorType type) noexcept;

	ShaderBuffer(VRAM& vram, std::string_view name, CreateInfo const& info);

	template <bool IsArray = false, typename T>
	ShaderBuffer& write(T t);
	ShaderBuffer& update(DescriptorSet& out_set, u32 binding);
	ShaderBuffer& next();

  private:
	void resize(std::size_t size, std::size_t count);

	struct Storage {
		std::vector<RingBuffer<Buffer>> buffers;
		std::string name;
		vk::BufferUsageFlagBits usage = {};
		u32 rotateCount = 0;
		std::size_t elemSize = 0;
	};

	Storage m_storage;
	Ref<VRAM> m_vram;
};

struct ShaderBuffer::CreateInfo {
	vk::DescriptorType type = vk::DescriptorType::eUniformBuffer;
	u32 rotateCount = 2;
};

// impl

constexpr vk::BufferUsageFlagBits ShaderBuffer::usage(vk::DescriptorType type) noexcept {
	switch (type) {
	case vk::DescriptorType::eStorageBuffer:
		return vk::BufferUsageFlagBits::eStorageBuffer;
	default:
		return vk::BufferUsageFlagBits::eUniformBuffer;
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

template <bool IsArray, typename T>
ShaderBuffer& ShaderBuffer::write(T t) {
	using traits = detail::ShaderBufTraits<T, IsArray>;
	using value_type = typename traits::value_type;
	static_assert(std::is_trivial_v<value_type>, "value_type must be trivial");
	if constexpr (IsArray) {
		resize(sizeof(value_type), t.size());
		std::size_t idx = 0;
		for (auto const& x : t) {
			m_storage.buffers[idx++].get().write(&x, sizeof(value_type));
		}
	} else {
		resize(sizeof(value_type), 1);
		m_storage.buffers.front().get().write(&t, sizeof(value_type));
	}
	return *this;
}
} // namespace le::graphics
