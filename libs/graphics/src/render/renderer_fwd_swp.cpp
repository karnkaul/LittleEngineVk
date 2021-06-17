#include <graphics/context/device.hpp>
#include <graphics/render/renderer_fwd_swp.hpp>

namespace le::graphics {
RendererFwdSwp::RendererFwdSwp(not_null<Swapchain*> swapchain, Buffering buffering)
	: ARenderer(swapchain, buffering, swapchain->display().extent, swapchain->depthFormat()) {
	ENSURE(hasDepthImage(), "RendererFS requires depth image");
	m_storage = make();
}

std::optional<RendererFwdSwp::Draw> RendererFwdSwp::beginFrame() {
	auto& buf = m_storage.buf.get();
	auto acquire = m_fence.acquire(*m_swapchain, *buf.draw);
	if (!acquire) { return std::nullopt; }
	m_device->device().resetCommandPool(*buf.pool, {});
	auto depth = depthImage(m_swapchain->depthFormat(), m_swapchain->display().extent);
	ENSURE(depth, "Depth image lost!");
	RenderTarget const target{acquire->image, *depth};
	buf.cb.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	return Draw{target, buf.cb};
}

void RendererFwdSwp::beginDraw(RenderTarget const& target, FrameDrawer& drawer, RGBA clear, vk::ClearDepthStencilValue depth) {
	auto const cl = clear.toVec4();
	vk::ClearColorValue const c = std::array{cl.x, cl.y, cl.z, cl.w};
	graphics::CommandBuffer::PassInfo const info{{c, depth}, vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
	auto& buf = m_storage.buf.get();
	buf.cb.transitionImage(target.colour.image, 1, vk::ImageAspectFlagBits::eColor, {vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal},
						   {{}, vAFB::eColorAttachmentWrite}, {vPSFB::eTopOfPipe, vPSFB::eColorAttachmentOutput});
	buf.framebuffer = makeDeferred<vk::Framebuffer>(m_device, *m_storage.renderPass, target.attachments(), cast(target.colour.extent), 1U);
	buf.cb.beginRenderPass(*m_storage.renderPass, *buf.framebuffer, target.colour.extent, info);
	buf.cb.setViewport(viewport(target.colour.extent, m_viewport.rect, m_viewport.offset));
	buf.cb.setScissor(scissor(target.colour.extent, m_viewport.rect, m_viewport.offset));
	drawer.draw3D(buf.cb);
	drawer.drawUI(buf.cb);
}

void RendererFwdSwp::endDraw(RenderTarget const& target) {
	auto& buf = m_storage.buf.get();
	buf.cb.endRenderPass();
	buf.cb.transitionImage(target.colour.image, 1, vk::ImageAspectFlagBits::eColor, {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR},
						   {vAFB::eColorAttachmentWrite, {}}, {vPSFB::eColorAttachmentOutput, vPSFB::eBottomOfPipe});
}

void RendererFwdSwp::endFrame() {
	auto& buf = m_storage.buf.get();
	buf.cb.end();
}

bool RendererFwdSwp::submitFrame() {
	auto& buf = m_storage.buf.get();
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags const waitStages = vPSFB::eTopOfPipe;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &buf.draw.m_t;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buf.cb.m_cb;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &buf.present.m_t;
	if (!m_fence.present(*m_swapchain, submitInfo, *buf.present)) { return false; }
	m_storage.buf.next();
	return true;
}

RendererFwdSwp::Storage RendererFwdSwp::make() const {
	Storage ret;
	for (u8 i = 0; i < m_fence.buffering().value; ++i) {
		Buf buf2(m_device, vk::CommandPoolCreateFlagBits::eTransient);
		ret.buf.push(std::move(buf2));
	}
	Attachment colour, depth;
	colour.format = m_swapchain->colourFormat().format;
	depth.format = m_swapchain->depthFormat();
	colour.layouts = {vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR};
	colour.layouts = {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal};
	depth.layouts = {vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal};
	ret.renderPass = {m_device, makeRenderPass(colour, depth, {})};
	return ret;
}
} // namespace le::graphics
