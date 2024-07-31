#include <levk/command_buffer.hpp>
#include <levk/core/error.hpp>

namespace levk {
CommandBuffer::CommandBuffer(IRenderDevice const& render_device)
	: m_pool(render_device.get_device().createCommandPoolUnique({vk::CommandPoolCreateFlagBits::eTransient})) {
	auto const cbai = vk::CommandBufferAllocateInfo{*m_pool, vk::CommandBufferLevel::ePrimary, 1};
	if (render_device.get_device().allocateCommandBuffers(&cbai, &m_cb) != vk::Result::eSuccess) { throw Error{"Failed to allocate Vulkan Command Buffer"}; }
	m_cb.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
}

auto CommandBuffer::submit(IRenderDevice& render_device) -> void {
	if (!m_cb) { return; }
	m_cb.end();
	auto const cbsi = vk::CommandBufferSubmitInfo{m_cb};
	auto si = vk::SubmitInfo2{};
	si.pCommandBufferInfos = &cbsi;
	si.commandBufferInfoCount = 1;
	auto fence = render_device.get_device().createFenceUnique({});
	render_device.queue_submit(si, *fence);
	auto const result = render_device.get_device().waitForFences(*fence, vk::True, std::numeric_limits<std::uint64_t>::max());
	if (result != vk::Result::eSuccess) { throw Error{"Failed to submit Vulkan Fence"}; }
	m_cb = vk::CommandBuffer{};
}
} // namespace levk
