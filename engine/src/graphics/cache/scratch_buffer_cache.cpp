#include <le/graphics/cache/scratch_buffer_cache.hpp>
#include <le/graphics/renderer.hpp>

namespace le::graphics {
auto ScratchBufferCache::allocate_host(vk::BufferUsageFlags const usage) -> HostBuffer& {
	auto& pool = m_maps[Renderer::self().get_frame_index()][usage];
	if (pool.next >= pool.buffers.size()) { pool.buffers.push_back(std::make_unique<HostBuffer>(usage)); }
	return *pool.buffers[pool.next++];
}

auto ScratchBufferCache::get_empty_buffer(vk::BufferUsageFlags const usage) -> DeviceBuffer const& {
	auto& pool = m_maps[Renderer::self().get_frame_index()][usage];
	if (!pool.empty_buffer) {
		pool.empty_buffer = std::make_unique<DeviceBuffer>(usage, 1);
		static constexpr std::uint8_t empty_byte_v{0x0};
		pool.empty_buffer->write(&empty_byte_v, sizeof(empty_byte_v));
	}
	return *pool.empty_buffer;
}

auto ScratchBufferCache::next_frame() -> void {
	for (auto& [_, pool] : m_maps[Renderer::self().get_frame_index()]) { pool.next = {}; }
}

auto ScratchBufferCache::clear() -> void { m_maps = {}; }
} // namespace le::graphics
