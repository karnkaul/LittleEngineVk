#include <graphics/context/device.hpp>
#include <graphics/render/fwd_swp_renderer.hpp>

namespace le::graphics {
RendererFS::RendererFS(not_null<Swapchain*> swapchain, Buffering buffering)
	: ARenderer(swapchain, buffering, swapchain->display().extent, swapchain->depthFormat()) {
	ENSURE(hasDepthImage(), "RendererFS requires depth image");
	m_attachments.colour.format = m_swapchain->colourFormat().format;
	m_attachments.depth.format = m_swapchain->depthFormat();
	m_storage = make();
}

RendererFS::~RendererFS() { destroy(); }

std::optional<RendererFS::Draw> RendererFS::beginFrame(CommandBuffer::PassInfo const& info) {
	auto& buf = m_storage.buf.get();
	auto acquire = m_fence.acquire(*m_swapchain, buf.semaphore);
	if (!acquire) { return std::nullopt; }
	m_device->destroy(buf.framebuffer);
	auto depth = depthImage(m_swapchain->depthFormat(), m_swapchain->display().extent);
	ENSURE(depth, "Depth image lost!");
	RenderTarget const target{acquire->image, *depth};
	buf.framebuffer = m_device->makeFramebuffer(m_storage.renderPass, target.attachments(), target.colour.extent);
	if (!buf.command.commandBuffer.begin(m_storage.renderPass, buf.framebuffer, target.colour.extent, info)) {
		ENSURE(false, "Failed to begin recording command buffer");
		m_device->defer([d = m_device, s = buf.semaphore.draw]() mutable { d->destroy(s); });
		buf.semaphore.draw = m_device->makeSemaphore(); // draw semaphore will be signalled by acquireNextImage and cannot be reused
		return std::nullopt;
	}
	return Draw{target, buf.command.commandBuffer};
}

bool RendererFS::endFrame() {
	auto& buf = m_storage.buf.get();
	buf.command.commandBuffer.end();
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags const waitStages = vk::PipelineStageFlagBits::eTopOfPipe;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &buf.semaphore.draw;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buf.command.commandBuffer.m_cb;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &buf.semaphore.present;
	if (!m_fence.present(*m_swapchain, submitInfo, buf.semaphore)) { return false; }
	m_storage.buf.next();
	return true;
}

RendererFS::Storage RendererFS::make() const {
	Storage ret;
	for (u8 i = 0; i < m_fence.buffering().value; ++i) { ret.buf.push({makeCommand(), makeSemaphore(), {}}); }
	ret.renderPass = makeRenderPass(m_attachments.colour, m_attachments.depth, {});
	return ret;
}

void RendererFS::destroy() {
	m_device->defer([d = m_device, s = m_storage]() mutable {
		for (auto& st : s.buf.ts) { d->destroy(st.semaphore.draw, st.semaphore.present, st.framebuffer, st.command.pool); }
		d->destroy(s.renderPass);
	});
	m_storage = {};
}
} // namespace le::graphics
