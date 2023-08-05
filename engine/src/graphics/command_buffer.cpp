#include <le/error.hpp>
#include <le/graphics/command_buffer.hpp>
#include <le/graphics/device.hpp>

namespace le::graphics {
CommandBuffer::CommandBuffer() {
	auto device = Device::self().get_device();
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
	auto fence = device.get_device().createFenceUnique({});
	device.submit(vsi, *fence);
	device.wait_for(*fence);
	m_cb = vk::CommandBuffer{};
}
} // namespace le::graphics
