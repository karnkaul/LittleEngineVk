#include <levk/core/utils/expect.hpp>
#include <levk/graphics/render/render_pass.hpp>
#include <levk/graphics/render/renderer.hpp>

namespace le::graphics {
RenderPass::RenderPass(not_null<Device*> device, not_null<PipelineFactory*> factory, RenderInfo info)
	: m_info(std::move(info)), m_device(device), m_factory(factory) {
	EXPECT(m_info.primary.recording() && m_info.framebuffer.colour.image && m_info.renderPass);
	if (!m_info.primary.recording() || !m_info.framebuffer.colour.image || !m_info.renderPass) { return; }
	bool const hasDepth = m_info.framebuffer.depth.image != vk::Image();
	vk::ImageView const colourDepth[] = {m_info.framebuffer.colour.view, m_info.framebuffer.depth.view};
	auto const views = hasDepth ? Span(colourDepth) : m_info.framebuffer.colour.view;
	auto const extent = m_info.framebuffer.extent();
	m_framebuffer = m_framebuffer.make(m_device->makeFramebuffer(m_info.renderPass, views, graphics::cast(extent), 1U), m_device);
	vk::CommandBufferInheritanceInfo inh;
	inh.renderPass = m_info.renderPass;
	inh.framebuffer = m_framebuffer;
	for (auto& cmd : m_info.secondary) {
		cmd.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inh);
	}
	if (m_info.secondary.empty()) { beginPass(); }
}

void RenderPass::end() {
	if (!m_info.secondary.empty()) {
		for (auto& cmd : m_info.secondary) { cmd.end(); }
		beginPass();
		ktl::fixed_vector<vk::CommandBuffer, max_secondary_cmd_v> secondary;
		for (auto const& cmd : m_info.secondary) { secondary.push_back(cmd.m_cb); }
		m_info.primary.m_cb.executeCommands((u32)secondary.size(), secondary.data());
	}
	m_info.primary.endRenderPass();
}

vk::Viewport RenderPass::viewport() const { return Renderer::viewport(framebuffer().extent(), view()); }
vk::Rect2D RenderPass::scissor() const { return Renderer::scissor(framebuffer().extent(), view()); }

void RenderPass::beginPass() {
	auto const cc = m_info.begin.clear.toVec4();
	vk::ClearColorValue const clear = std::array{cc.x, cc.y, cc.z, cc.w};
	m_device->m_layouts.transition(m_info.primary.m_cb, m_info.framebuffer.colour.image, vIL::eColorAttachmentOptimal, LayoutStages::topColour());
	if (m_info.framebuffer.depth.image != vk::Image()) {
		m_device->m_layouts.transition(m_info.primary.m_cb, m_info.framebuffer.depth.image, vIL::eDepthStencilAttachmentOptimal, LayoutStages::topDepth());
	}
	graphics::CommandBuffer::PassInfo passInfo;
	passInfo.clearValues = {clear, m_info.begin.depth};
	passInfo.subpassContents = m_info.secondary.empty() ? vk::SubpassContents::eInline : vk::SubpassContents::eSecondaryCommandBuffers;
	passInfo.usage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	m_info.primary.beginRenderPass(m_info.renderPass, m_framebuffer, m_info.framebuffer.extent(), passInfo);
}
} // namespace le::graphics
