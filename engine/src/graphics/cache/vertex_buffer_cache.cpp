#include <spaced/core/logger.hpp>
#include <spaced/graphics/cache/vertex_buffer_cache.hpp>
#include <spaced/graphics/device.hpp>
#include <spaced/graphics/renderer.hpp>

namespace spaced::graphics {
namespace {
constexpr auto usage_v = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer;

auto make_buffered() -> Buffered<std::shared_ptr<HostBuffer>> {
	auto ret = Buffered<std::shared_ptr<HostBuffer>>{};
	fill_buffered(ret, [] { return std::make_shared<HostBuffer>(usage_v); });
	return ret;
}

auto const g_log{logger::Logger{"Cache"}};
} // namespace

auto VertexBufferCache::allocate() -> Buffered<std::shared_ptr<HostBuffer>> {
	for (auto const& buffer : m_buffers) {
		if (buffer[0].use_count() == 1) { return buffer; }
	}

	m_buffers.push_back(make_buffered());
	g_log.debug("new Vulkan Vertex Buffer created (total: {})", m_buffers.size());

	return m_buffers.back();
}

auto VertexBufferCache::clear() -> void {
	Device::self().device().waitIdle();
	g_log.debug("{} Vertex Buffers destroyed", m_buffers.size());
	m_buffers.clear();
}
} // namespace spaced::graphics
