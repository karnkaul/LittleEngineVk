#include <graphics/command_buffer.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/memory.hpp>
#include <graphics/render/descriptor_set.hpp>

namespace le::graphics {
std::vector<CommandBuffer> CommandBuffer::make(not_null<Device*> device, vk::CommandPool pool, u32 count) {
	std::vector<CommandBuffer> ret;
	make(ret, device, pool, count);
	return ret;
}

void CommandBuffer::make(std::vector<CommandBuffer>& out, not_null<Device*> device, vk::CommandPool pool, u32 count) {
	vk::CommandBufferAllocateInfo allocInfo(pool, vk::CommandBufferLevel::ePrimary, count);
	auto buffers = device->device().allocateCommandBuffers(allocInfo);
	out.reserve(out.size() + buffers.size());
	for (auto const cb : buffers) { out.push_back(CommandBuffer(cb)); }
}

CommandBuffer::CommandBuffer(vk::CommandBuffer cmd) : m_cb(cmd) { ENSURE(!Device::default_v(cmd), "Null command buffer!"); }

CommandBuffer::CommandBuffer(Device& device, vk::CommandPool pool, vk::CommandBufferLevel level) {
	vk::CommandBufferAllocateInfo allocInfo(pool, level, 1U);
	auto buffers = device.device().allocateCommandBuffers(allocInfo);
	m_cb = buffers.front();
}

void CommandBuffer::begin(vk::CommandBufferUsageFlags usage, vk::CommandBufferInheritanceInfo const* inheritance) {
	ENSURE(valid() && !recording() && !rendering(), "Invalid command buffer state");
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = usage;
	beginInfo.pInheritanceInfo = inheritance;
	m_cb.begin(beginInfo);
	m_flags.set(Flag::eRecording);
}

void CommandBuffer::beginRenderPass(vk::RenderPass renderPass, vk::Framebuffer framebuffer, Extent2D extent, PassInfo const& info) {
	ENSURE(valid() && recording() && !rendering(), "Invalid command buffer state");
	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.extent = cast(extent);
	renderPassInfo.clearValueCount = (u32)info.clearValues.size();
	renderPassInfo.pClearValues = info.clearValues.data();
	m_cb.beginRenderPass(renderPassInfo, info.subpassContents);
	m_flags.set(Flag::eRendering);
}

void CommandBuffer::setViewport(vk::Viewport viewport) const {
	ENSURE(recording(), "Command buffer not recording!");
	m_cb.setViewport(0, viewport);
}

void CommandBuffer::setScissor(vk::Rect2D scissor) const {
	ENSURE(recording(), "Command buffer not recording!");
	m_cb.setScissor(0, scissor);
}

void CommandBuffer::setViewportScissor(vk::Viewport viewport, vk::Rect2D scissor) const {
	ENSURE(recording(), "Command buffer not recording!");
	m_cb.setViewport(0, viewport);
	m_cb.setScissor(0, scissor);
}

void CommandBuffer::bind(vk::Pipeline pipeline, vBP bindPoint) const {
	ENSURE(recording(), "Command buffer not recording!");
	m_cb.bindPipeline(bindPoint, pipeline);
}

void CommandBuffer::bindSets(vk::PipelineLayout layout, vAP<vk::DescriptorSet> sets, u32 firstSet, vAP<u32> offsets, vBP bindPoint) const {
	ENSURE(recording(), "Command buffer not recording!");
	m_cb.bindDescriptorSets(bindPoint, layout, firstSet, sets, offsets);
}

void CommandBuffer::bindSet(vk::PipelineLayout layout, DescriptorSet const& set) const { bindSets(layout, set.get(), set.setNumber()); }

void CommandBuffer::bindVBOs(u32 first, vAP<vk::Buffer> buffers, vAP<vk::DeviceSize> offsets) const {
	ENSURE(recording(), "Command buffer not recording!");
	m_cb.bindVertexBuffers(first, buffers, offsets);
}

void CommandBuffer::bindIBO(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType indexType) const {
	ENSURE(recording(), "Command buffer not recording!");
	m_cb.bindIndexBuffer(buffer, offset, indexType);
}

void CommandBuffer::bindVBO(Buffer const& vbo, Buffer const* pIbo) const {
	ENSURE(recording(), "Command buffer not recording!");
	bindVBOs(0, vbo.buffer(), vk::DeviceSize(0));
	if (pIbo) { bindIBO(pIbo->buffer()); }
}

void CommandBuffer::drawIndexed(u32 indexCount, u32 instanceCount, u32 firstInstance, s32 vertexOffset, u32 firstIndex) const {
	ENSURE(recording(), "Command buffer not recording!");
	m_cb.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	s_drawCalls.fetch_add(1);
}

void CommandBuffer::draw(u32 vertexCount, u32 instanceCount, u32 firstInstance, u32 firstVertex) const {
	ENSURE(recording(), "Command buffer not recording!");
	m_cb.draw(vertexCount, instanceCount, firstVertex, firstInstance);
	s_drawCalls.fetch_add(1);
}

void CommandBuffer::transitionImage(Image const& image, vk::ImageAspectFlags aspect, Layouts transition, Access access, Stages stages) const {
	transitionImage(image.image(), image.layerCount(), aspect, transition, access, stages);
}

void CommandBuffer::transitionImage(vk::Image image, u32 layerCount, vk::ImageAspectFlags aspect, Layouts transition, Access access, Stages stages) const {
	ENSURE(recording(), "Command buffer not recording!");
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = transition.first;
	barrier.newLayout = transition.second;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;
	barrier.srcAccessMask = access.first;
	barrier.dstAccessMask = access.second;
	m_cb.pipelineBarrier(stages.first, stages.second, {}, {}, {}, barrier);
}

void CommandBuffer::endRenderPass() {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cb.endRenderPass();
	m_flags.reset(Flag::eRendering);
}

void CommandBuffer::end() {
	ENSURE(recording() && !rendering(), "Command buffer not recording!");
	m_cb.end();
	m_flags.reset(Flag::eRecording);
}
} // namespace le::graphics
