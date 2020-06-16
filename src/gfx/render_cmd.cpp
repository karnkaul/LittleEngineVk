#include <gfx/render_cmd.hpp>

namespace le::gfx
{
RenderCmd::RenderCmd(vk::CommandBuffer commandBuffer) : m_commandBuffer(commandBuffer) {}

RenderCmd::RenderCmd(vk::CommandBuffer commandBuffer, vk::RenderPass renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent,
					 vk::ArrayProxy<vk::ClearValue const> clearValues, vk::SubpassContents subpassContents)
	: m_commandBuffer(commandBuffer)
{
	begin(renderPass, framebuffer, extent, clearValues, subpassContents);
}

RenderCmd::RenderCmd(RenderCmd&&) = default;
RenderCmd& RenderCmd::operator=(RenderCmd&&) = default;

RenderCmd::~RenderCmd()
{
	end();
}

void RenderCmd::begin(vk::RenderPass renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent,
					  vk::ArrayProxy<vk::ClearValue const> clearValues, vk::SubpassContents subpassContents)
{
	ASSERT(m_commandBuffer != vk::CommandBuffer(), "Null command buffer!");
	ASSERT(!m_bRecording, "Command buffer already recording!");
	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.extent = extent;
	renderPassInfo.clearValueCount = clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	// Begin
	vk::CommandBufferBeginInfo beginInfo;
	m_commandBuffer.begin(beginInfo);
	m_commandBuffer.beginRenderPass(renderPassInfo, subpassContents);
	m_bRecording = true;
}

void RenderCmd::setViewportScissor(vk::Viewport viewport, vk::Rect2D scissor) const
{
	ASSERT(m_commandBuffer != vk::CommandBuffer(), "Null command buffer!");
	ASSERT(m_bRecording, "Command buffer not recording!");
	m_commandBuffer.setViewport(0, viewport);
	m_commandBuffer.setScissor(0, scissor);
}

void RenderCmd::bindVertexBuffers(u32 first, vk::ArrayProxy<vk::Buffer const> buffers, vk::ArrayProxy<vk::DeviceSize const> offsets) const
{
	ASSERT(m_commandBuffer != vk::CommandBuffer(), "Null command buffer!");
	ASSERT(m_bRecording, "Command buffer not recording!");
	m_commandBuffer.bindVertexBuffers(first, buffers, offsets);
}

void RenderCmd::bindIndexBuffer(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType indexType) const
{
	ASSERT(m_commandBuffer != vk::CommandBuffer(), "Null command buffer!");
	ASSERT(m_bRecording, "Command buffer not recording!");
	m_commandBuffer.bindIndexBuffer(buffer, offset, indexType);
}

void RenderCmd::drawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, s32 vertexOffset, u32 firstInstance) const
{
	ASSERT(m_commandBuffer != vk::CommandBuffer(), "Null command buffer!");
	ASSERT(m_bRecording, "Command buffer not recording!");
	m_commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void RenderCmd::draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) const
{
	ASSERT(m_commandBuffer != vk::CommandBuffer(), "Null command buffer!");
	ASSERT(m_bRecording, "Command buffer not recording!");
	m_commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void RenderCmd::end()
{
	ASSERT(m_commandBuffer != vk::CommandBuffer(), "Null command buffer!");
	ASSERT(m_bRecording, "Command buffer not recording!");
	m_commandBuffer.endRenderPass();
	m_commandBuffer.end();
	m_bRecording = false;
}
} // namespace le::gfx
