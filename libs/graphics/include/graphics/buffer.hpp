#pragma once
#include <core/utils/expect.hpp>
#include <graphics/memory.hpp>
#include <graphics/utils/defer.hpp>

namespace le::graphics {
class Buffer {
  public:
	enum class Type { eCpuToGpu, eGpuOnly };
	struct CreateInfo;

	static constexpr auto allocation_type_v = Memory::Type::eBuffer;

	Buffer(not_null<Memory*> memory, CreateInfo const& info);

	vk::Buffer buffer() const noexcept { return m_buffer.get().resource.get<vk::Buffer>(); }
	vk::DeviceSize writeSize() const noexcept { return m_buffer.get().size; }
	std::size_t writeCount() const noexcept { return m_data.writeCount; }
	vk::BufferUsageFlags usage() const noexcept { return m_data.usage; }
	Type bufferType() const noexcept { return m_data.type; }

	void const* mapped() const noexcept { return m_buffer.get().data; }
	void const* map();
	bool unmap();
	bool write(void const* data, vk::DeviceSize size = 0, vk::DeviceSize offset = 0);
	template <typename T>
	bool writeT(T const& t, vk::DeviceSize offset = 0);

  private:
	struct Data {
		std::size_t writeCount = 0;
		vk::BufferUsageFlags usage;
		Type type{};
	};
	Defer<Memory::Resource, Memory, Memory::Deleter> m_buffer;
	Data m_data;
	not_null<Memory*> m_memory;

	friend class VRAM;
};

struct Buffer::CreateInfo : Memory::AllocInfo {
	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags properties;
};

// impl

template <typename T>
bool Buffer::writeT(T const& t, vk::DeviceSize offset) {
	EXPECT(sizeof(T) + offset <= m_buffer.get().size);
	return write(&t, sizeof(T), offset);
}
} // namespace le::graphics
