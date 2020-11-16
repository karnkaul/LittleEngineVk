#include <graphics/context/command_buffer.hpp>
#include <graphics/context/device.hpp>
#include <graphics/pipeline.hpp>

namespace le::graphics {
std::vector<CommandBuffer> CommandBuffer::make(Device& device, vk::CommandPool pool, u32 count, bool bSecondary) {
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = pool;
	allocInfo.level = bSecondary ? vk::CommandBufferLevel::eSecondary : vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = count;
	auto buffers = device.m_device.allocateCommandBuffers(allocInfo);
	std::vector<CommandBuffer> ret;
	for (auto& buffer : buffers) {
		ret.push_back({buffer, pool});
	}
	return ret;
}

CommandBuffer::CommandBuffer(vk::CommandBuffer cmd, vk::CommandPool pool) : m_cmd(cmd), m_pool(pool) {
	ENSURE(!default_v(cmd), "Null command buffer!");
}

bool CommandBuffer::begin(vk::CommandBufferUsageFlags usage) {
	ENSURE(m_flags.none(Flag::eRecording), "Command buffer already recording!");
	if (valid() && !recording()) {
		vk::CommandBufferBeginInfo beginInfo;
		beginInfo.flags = usage;
		m_cmd.begin(beginInfo);
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
		m_cmd.beginRenderPass(renderPassInfo, info.subpassContents);
		m_flags.set(Flag::eRendering);
		return true;
	}
	return false;
}

void CommandBuffer::setViewportScissor(vk::Viewport viewport, vk::Rect2D scissor) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cmd.setViewport(0, viewport);
	m_cmd.setScissor(0, scissor);
}

void CommandBuffer::bindPipe(Pipeline const& pipeline) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cmd.bindPipeline(pipeline.m_metadata.createInfo.fixedState.bindPoint, pipeline.m_storage.dynamic.pipeline);
}

void CommandBuffer::bind(vk::Pipeline pipeline, vBP bindPoint) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cmd.bindPipeline(bindPoint, pipeline);
}

void CommandBuffer::bindSets(vk::PipelineLayout layout, vAP<vk::DescriptorSet> sets, u32 firstSet, vAP<u32> offsets, vBP bindPoint) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cmd.bindDescriptorSets(bindPoint, layout, firstSet, sets, offsets);
}

void CommandBuffer::bindVBOs(u32 first, vAP<vk::Buffer> buffers, vAP<vk::DeviceSize> offsets) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cmd.bindVertexBuffers(first, buffers, offsets);
}

void CommandBuffer::bindIBO(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType indexType) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cmd.bindIndexBuffer(buffer, offset, indexType);
}

void CommandBuffer::bindVBO(CView<Buffer> vbo, CView<Buffer> ibo) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	ENSURE(vbo, "Null view(s)");
	bindVBOs(0, vbo->buffer, vk::DeviceSize(0));
	if (ibo.valid()) {
		bindIBO(ibo->buffer);
	}
}

void CommandBuffer::drawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, s32 vertexOffset, u32 firstInstance) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cmd.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) const {
	ENSURE(rendering(), "Command buffer not rendering!");
	m_cmd.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::end() {
	ENSURE(recording(), "Command buffer not recording!");
	if (m_flags.test(Flag::eRendering)) {
		m_cmd.endRenderPass();
		m_flags.reset(Flag::eRendering);
	}
	if (m_flags.test(Flag::eRecording)) {
		m_cmd.end();
		m_flags.reset(Flag::eRecording);
	}
}
} // namespace le::graphics
