#include <graphics/context/device.hpp>
#include <graphics/render/renderer_fwd_swp.hpp>

namespace le::graphics {
RendererFwdSwp::RendererFwdSwp(not_null<Swapchain*> swapchain, Buffering buffering)
	: ARenderer(swapchain, buffering, swapchain->display().extent, swapchain->depthFormat()) {
	ensure(hasDepthImage(), "RendererFS requires depth image");
	m_storage = make();
}

std::optional<RendererFwdSwp::Draw> RendererFwdSwp::beginFrame() {
	auto& buf = m_storage.buf.get();
	auto acquire = m_fence.acquire(*m_swapchain, *buf.draw);
	if (!acquire) { return std::nullopt; }
	m_storage.swapImg = acquire->image;
	m_device->device().resetCommandPool(*buf.pool, {});
	auto depth = depthImage(m_swapchain->depthFormat(), m_swapchain->display().extent);
	ensure(depth.has_value(), "Depth image lost!");
	Image& os = offscreen(buf, acquire->image.extent);
	RenderImage const colour{os.image(), os.view(), cast(os.extent())};
	RenderTarget const target{colour, *depth};
	buf.cb.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	return Draw{target, buf.cb};
}

void RendererFwdSwp::beginDraw(RenderTarget const& target, FrameDrawer& drawer, RGBA clear, vk::ClearDepthStencilValue depth) {
	auto const cl = clear.toVec4();
	vk::ClearColorValue const c = std::array{cl.x, cl.y, cl.z, cl.w};
	graphics::CommandBuffer::PassInfo const info{{c, depth}, vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
	auto& buf = m_storage.buf.get();
	buf.framebuffer = makeDeferred<vk::Framebuffer>(m_device, *m_storage.renderPass, target.attachments(), cast(target.colour.extent), 1U);
	m_device->m_layouts.transition<lt::ColourWrite>(buf.cb, target.colour.image);
	m_device->m_layouts.transition<lt::DepthStencilWrite>(buf.cb, target.depth.image, depthStencil);
	buf.cb.beginRenderPass(*m_storage.renderPass, *buf.framebuffer, target.colour.extent, info);
	buf.cb.setViewport(viewport(target.colour.extent, m_viewport.rect, m_viewport.offset));
	buf.cb.setScissor(scissor(target.colour.extent, m_viewport.rect, m_viewport.offset));
	drawer.draw3D(buf.cb);
	drawer.drawUI(buf.cb);
}

void RendererFwdSwp::endDraw(RenderTarget const& target) {
	auto& buf = m_storage.buf.get();
	buf.cb.endRenderPass();
	m_device->m_layouts.transition<lt::TransferSrc>(buf.cb, target.colour.image);
	m_device->m_layouts.transition<lt::TransferDst>(buf.cb, m_storage.swapImg.image);
	vk::Extent3D off = vk::Extent3D{cast(target.colour.extent), 1};
	vk::Extent3D swp = vk::Extent3D{cast(m_storage.swapImg.extent), 1};
	m_swapchain->m_vram->blit(buf.cb.m_cb, target.colour.image, m_storage.swapImg.image, {off, swp});
	m_device->m_layouts.transition<lt::TransferPresent>(buf.cb, m_storage.swapImg.image);
	m_device->m_layouts.drawn(target.depth.image);
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

Image& RendererFwdSwp::offscreen(Buf& buf, Extent2D extent) {
	if (!buf.offscreen || cast(buf.offscreen->extent()) != extent) {
		Image::CreateInfo info;
		info.createInfo.format = vk::Format::eR8G8B8A8Unorm;
		info.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		info.createInfo.extent = vk::Extent3D(cast(extent), 1);
		info.createInfo.tiling = vk::ImageTiling::eOptimal;
		info.createInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
		info.createInfo.samples = vk::SampleCountFlagBits::e1;
		info.createInfo.imageType = vk::ImageType::e2D;
		info.createInfo.initialLayout = vIL::eUndefined;
		info.createInfo.mipLevels = 1;
		info.createInfo.arrayLayers = 1;
		info.queueFlags = QFlags(QType::eTransfer) | QType::eGraphics;
		info.view.format = info.createInfo.format;
		info.view.aspects = vk::ImageAspectFlagBits::eColor;
		buf.offscreen = Image(m_swapchain->m_vram, info);
	}
	return *buf.offscreen;
}

RendererFwdSwp::Storage RendererFwdSwp::make() const {
	Storage ret;
	for (u8 i = 0; i < m_fence.buffering().value; ++i) {
		Buf buf2(m_device, vk::CommandPoolCreateFlagBits::eTransient);
		ret.buf.push(std::move(buf2));
	}
	Attachment colour, depth;
	colour.format = vk::Format::eR8G8B8A8Unorm;
	depth.format = m_swapchain->depthFormat();
	colour.layouts = {vIL::eUndefined, vIL::ePresentSrcKHR};
	colour.layouts = {vIL::eColorAttachmentOptimal, vIL::eColorAttachmentOptimal};
	depth.layouts = {vIL::eUndefined, vIL::eDepthStencilAttachmentOptimal};
	depth.layouts = {vIL::eDepthStencilAttachmentOptimal, vIL::eDepthStencilAttachmentOptimal};
	ret.renderPass = {m_device, makeRenderPass(colour, depth, {})};
	return ret;
}
} // namespace le::graphics
