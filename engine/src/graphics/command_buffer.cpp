#include <spaced/engine/error.hpp>
#include <spaced/engine/graphics/command_buffer.hpp>
#include <spaced/engine/graphics/device.hpp>

namespace spaced::graphics {
CommandBuffer::CommandBuffer() {
	auto device = Device::self().device();
	m_pool = device.createCommandPoolUnique({vk::CommandPoolCreateFlagBits::eTransient});
	auto const cbai = vk::CommandBufferAllocateInfo{*m_pool, vk::CommandBufferLevel::ePrimary, 1};
	if (device.allocateCommandBuffers(&cbai, &m_cb) != vk::Result::eSuccess) { throw Error{"Failed to allocate Vulkan Command Buffer"}; }
	m_cb.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
}

auto CommandBuffer::submit() -> void {
	if (!m_cb) { return; }
	m_cb.end();
	auto const cbsi = vk::CommandBufferSubmitInfo{m_cb};
	auto vsi = vk::SubmitInfo2{};
	vsi.commandBufferInfoCount = 1;
	vsi.pCommandBufferInfos = &cbsi;
	auto& device = Device::self();
	auto fence = device.device().createFenceUnique({});
	device.submit(vsi, *fence);
	device.wait_for(*fence);
	m_cb = vk::CommandBuffer{};
}
} // namespace spaced::graphics
