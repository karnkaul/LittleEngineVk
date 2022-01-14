#pragma once
#include <core/span.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/memory.hpp>
#include <graphics/utils/trotator.hpp>

namespace le::graphics {
class DescriptorSet;

class ShaderBuffer {
  public:
	struct CreateInfo;

	ShaderBuffer() = default;
	ShaderBuffer(not_null<VRAM*> vram, CreateInfo const& info);

	template <typename T>
	ShaderBuffer& write(T const& t, std::size_t offset = 0);
	template <typename T>
	ShaderBuffer& writeArray(T const& t);
	ShaderBuffer& write(void const* data, std::size_t size, std::size_t offset);
	ShaderBuffer const& update(DescriptorSet& out_set, u32 binding) const;
	ShaderBuffer& swap();

	bool valid() const noexcept;
	vk::DescriptorType type() const noexcept;

  private:
	void resize(std::size_t size, std::size_t count);

	struct Storage {
		std::vector<TRotator<Buffer>> buffers;
		vk::DescriptorType type;
		vk::BufferUsageFlagBits usage = {};
		Buffering buffering;
		std::size_t elemSize = 0;
	};

	Storage m_storage;
	VRAM* m_vram = {};
};

struct ShaderBuffer::CreateInfo {
	vk::DescriptorType type = vk::DescriptorType::eUniformBuffer;
	Buffering buffering = DeferQueue::defaultDefer;
};

// impl

inline bool ShaderBuffer::valid() const noexcept { return m_vram != nullptr; }
inline vk::DescriptorType ShaderBuffer::type() const noexcept { return m_storage.type; }

template <typename T>
ShaderBuffer& ShaderBuffer::write(T const& t, std::size_t offset) {
	resize(sizeof(T), 1);
	m_storage.buffers.front().get().write(&t, sizeof(T), (vk::DeviceSize)offset);
	return *this;
}

template <typename T>
ShaderBuffer& ShaderBuffer::writeArray(T const& t) {
	using value_type = typename T::value_type;
	resize(sizeof(value_type), t.size());
	std::size_t idx = 0;
	for (auto const& x : t) { m_storage.buffers[idx++].get().write(&x, sizeof(value_type)); }
	return *this;
}
} // namespace le::graphics
