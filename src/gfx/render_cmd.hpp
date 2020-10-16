#pragma once
#include <core/ensure.hpp>
#include <gfx/pipeline_impl.hpp>
#include <vulkan/vulkan.hpp>

namespace le::gfx {
class RenderCmd final {
  private:
	vk::CommandBuffer m_commandBuffer;
	mutable vk::Pipeline m_prevPipeline;
	bool m_bRecording = false;

  public:
	RenderCmd(vk::CommandBuffer commandBuffer);
	RenderCmd(vk::CommandBuffer commandBuffer, vk::RenderPass renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent,
			  vk::ArrayProxy<vk::ClearValue const> clearValues, vk::SubpassContents subpassContents = vk::SubpassContents::eInline);
	RenderCmd(RenderCmd&&);
	RenderCmd& operator=(RenderCmd&&);
	~RenderCmd();

  public:
	void begin(vk::RenderPass renderPass, vk::Framebuffer framebuffer, vk::Extent2D extent, vk::ArrayProxy<vk::ClearValue const> clearValues,
			   vk::SubpassContents subpassContents = vk::SubpassContents::eInline);

	void setViewportScissor(vk::Viewport viewport, vk::Rect2D scissor) const;

	template <typename T>
	void bindResources(PipelineImpl const& pipeline, vk::ArrayProxy<vk::DescriptorSet const> sets, vk::ShaderStageFlags stages, u32 offset,
					   vk::ArrayProxy<T const> pushConstants) const {
		ENSURE(m_commandBuffer != vk::CommandBuffer(), "Null command buffer!");
		ENSURE(m_bRecording, "Command buffer not recording!");
		if (pipeline.m_pipeline != m_prevPipeline) {
			m_commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.m_pipeline);
			m_prevPipeline = pipeline.m_pipeline;
		}
		m_commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.m_layout, 0, sets, {});
		m_commandBuffer.pushConstants<T>(pipeline.m_layout, stages, offset, pushConstants);
	}

	void bindVertexBuffers(u32 first, vk::ArrayProxy<vk::Buffer const> buffers, vk::ArrayProxy<vk::DeviceSize const> offsets) const;
	void bindIndexBuffer(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType indexType = vk::IndexType::eUint32) const;
	void drawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, s32 vertexOffset, u32 firstInstance) const;
	void draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) const;

	void end();
};
} // namespace le::gfx
