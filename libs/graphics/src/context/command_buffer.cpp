#include <graphics/common.hpp>
#include <graphics/context/command_buffer.hpp>
#include <graphics/context/device.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/resources.hpp>

namespace le::graphics {
std::vector<CommandBuffer> CommandBuffer::make(Device& device, vk::CommandPool pool, u32 count, bool bSecondary) {
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = pool;
	allocInfo.level = bSecondary ? vk::CommandBufferLevel::eSecondary : vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = count;
	auto buffers = device.device().allocateCommandBuffers(allocInfo);
	std::vector<CommandBuffer> ret;
	for (auto& buffer : buffers) {
		ret.push_back({buffer, pool});
	}
	return ret;
}

CommandBuffer::CommandBuffer(vk::CommandBuffer cmd, vk::CommandPool pool) : m_cb(cmd), m_pool(pool) {
	ENSURE(!Device::default_v(cmd), "Null command buffer!");
}

bool CommandBuffer::begin(vk::CommandBufferUsageFlags usage) {
	ENSURE(m_flags.none(Flag::eRecording), "Command buffer already recording!");
	if (valid() && !recording()) {
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = usage;
		m_cb.begin(beginInfo);
		m_flags.set(Flag::eRecording);
		return true;
	}
	return false;
}

bool CommandBuffer::begin(vk::RenderPass renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent, PassInfo const& info) {
	ENSURE(m_flags.none(Flag::eRendering), "Command buffer already rendering a pass!");
	if (valid() && !rendering()) {
		if (!recording() && !begin(info.usage)) {
			ENSURE(false, "Invariant violated");
		}
		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.extent = extent;
		renderPassInfo.clearValueCount = (u32)info.clearValues.size();
		renderPassInfo.pClearValues = info.clearValues.data();
		m_cb.beginRenderPass(renderPassInfo, info.subpassContents);
		m_flags.set(Flag::eRendering);
		return true;
	}
	return false;
}

void CommandBuffer::setViewportScissor(vk::Viewport viewport, vk::Rect2D scissor) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cb.setViewport(0, viewport);
	m_cb.setScissor(0, scissor);
}

void CommandBuffer::bindPipe(Pipeline const& pipeline) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cb.bindPipeline(pipeline.m_metadata.createInfo.fixedState.bindPoint, pipeline.m_storage.dynamic.pipeline);
}

void CommandBuffer::bind(vk::Pipeline pipeline, vBP bindPoint) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cb.bindPipeline(bindPoint, pipeline);
}

void CommandBuffer::bindSets(vk::PipelineLayout layout, vAP<vk::DescriptorSet> sets, u32 firstSet, vAP<u32> offsets, vBP bindPoint) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cb.bindDescriptorSets(bindPoint, layout, firstSet, sets, offsets);
}

void CommandBuffer::bindSet(vk::PipelineLayout layout, DescriptorSet const& set) const {
	bindSets(layout, set.get(), set.setNumber());
}

void CommandBuffer::bindVBOs(u32 first, vAP<vk::Buffer> buffers, vAP<vk::DeviceSize> offsets) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cb.bindVertexBuffers(first, buffers, offsets);
}

void CommandBuffer::bindIBO(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType indexType) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cb.bindIndexBuffer(buffer, offset, indexType);
}

void CommandBuffer::bindVBO(Buffer const& vbo, Buffer const* pIbo) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	bindVBOs(0, vbo.buffer(), vk::DeviceSize(0));
	if (pIbo) {
		bindIBO(pIbo->buffer());
	}
}

void CommandBuffer::drawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, s32 vertexOffset, u32 firstInstance) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cb.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cb.draw(vertexCount, instanceCount, firstVertex, firstInstance);
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
	if (m_flags.test(Flag::eRendering)) {
		m_cb.endRenderPass();
		m_flags.reset(Flag::eRendering);
	}
}

void CommandBuffer::end() {
	ENSURE(recording(), "Command buffer not recording!");
	if (m_flags.test(Flag::eRendering)) {
		m_cb.endRenderPass();
		m_flags.reset(Flag::eRendering);
	}
	if (m_flags.test(Flag::eRecording)) {
		m_cb.end();
		m_flags.reset(Flag::eRecording);
	}
}
} // namespace le::graphics
