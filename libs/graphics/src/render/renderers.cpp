#include <graphics/context/device.hpp>
#include <graphics/render/renderers.hpp>

namespace le::graphics {
RendererFSR::RendererFSR(not_null<Swapchain*> swapchain, Buffering buffering) : ARenderer(swapchain, buffering) { m_storage = make(tech_v.transition); }

CommandBuffer RendererFSR::beginDraw(RenderTarget const& target, ScreenView const& view, RGBA clear, ClearDepth depth) {
	auto& buf = m_storage.buf.get();
	auto const cl = clear.toVec4();
	vk::ClearColorValue const c = std::array{cl.x, cl.y, cl.z, cl.w};
	buf.framebuffer = makeDeferred<vk::Framebuffer>(m_device, m_storage.renderPass, target.attachments(), cast(target.colour.extent), 1U);
	graphics::CommandBuffer::PassInfo const info{{c, depth}, vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
	buf.cb.beginRenderPass(m_storage.renderPass, buf.framebuffer, target.colour.extent, info);
	buf.cb.setViewport(viewport(target.colour.extent, view));
	buf.cb.setScissor(scissor(target.colour.extent, view));
	return buf.cb;
}

void RendererFSR::endDraw(RenderTarget const& target) {
	auto& buf = m_storage.buf.get();
	buf.cb.endRenderPass();
	m_device->m_layouts.drawn(target.depth.image);
}

RendererFSC::RendererFSC(not_null<Swapchain*> swapchain, Buffering buffering) : RendererFSR(swapchain, buffering) { m_storage = make(tech_v.transition); }

CommandBuffer RendererFSC::beginDraw(RenderTarget const& target, ScreenView const& view, RGBA clear, ClearDepth depth) {
	auto& buf = m_storage.buf.get();
	m_device->m_layouts.transition<lt::ColourWrite>(buf.cb, target.colour.image);
	m_device->m_layouts.transition<lt::DepthStencilWrite>(buf.cb, target.depth.image, depthStencil);
	return RendererFSR::beginDraw(target, view, clear, depth);
}

void RendererFSC::endDraw(RenderTarget const& target) {
	RendererFSR::endDraw(target);
	m_device->m_layouts.transition<lt::TransferPresent>(m_storage.buf.get().cb, target.colour.image);
}

RendererFOC::RendererFOC(not_null<Swapchain*> swapchain, Buffering buffering) : RendererFSR(swapchain, buffering) {
	m_storage = make(tech_v.transition, {m_colourFormat, {}});
	Image::CreateInfo colourInfo;
	colourInfo.createInfo.format = m_colourFormat;
	colourInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	colourInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
	colourInfo.createInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	colourInfo.createInfo.samples = vk::SampleCountFlagBits::e1;
	colourInfo.createInfo.imageType = vk::ImageType::e2D;
	colourInfo.createInfo.initialLayout = vIL::eUndefined;
	colourInfo.createInfo.mipLevels = 1;
	colourInfo.createInfo.arrayLayers = 1;
	colourInfo.queueFlags = QFlags(QType::eTransfer) | QType::eGraphics;
	colourInfo.view.format = colourInfo.createInfo.format;
	colourInfo.view.aspects = vk::ImageAspectFlagBits::eColor;
	m_imageMaker.infos.push_back(colourInfo);
	m_colourIndex = m_imageMaker.infos.size() - 1;
	// renderScale(0.75f);
}

std::optional<RenderTarget> RendererFOC::beginFrame() {
	if (auto acq = acquire()) {
		auto& buf = m_storage.buf.get();
		m_swapchainImage = acq->image;
		auto const extent = scaleExtent(m_swapchainImage.extent, renderScale());
		auto depth = depthImage(extent);
		return RenderTarget{renderImage(m_imageMaker.refresh(buf.offscreen, extent, m_colourFormat, m_colourIndex)), depth};
	}
	return std::nullopt;
}

CommandBuffer RendererFOC::beginDraw(RenderTarget const& target, ScreenView const& view, RGBA clear, ClearDepth depth) {
	auto& buf = m_storage.buf.get();
	m_device->m_layouts.transition<lt::ColourWrite>(buf.cb, target.colour.image);
	m_device->m_layouts.transition<lt::DepthStencilWrite>(buf.cb, target.depth.image, depthStencil);
	return RendererFSR::beginDraw(target, view, clear, depth);
}

void RendererFOC::endDraw(RenderTarget const& target) {
	RendererFSR::endDraw(target);
	auto& buf = m_storage.buf.get();
	m_device->m_layouts.transition<lt::TransferSrc>(buf.cb, target.colour.image);
	m_device->m_layouts.transition<lt::TransferDst>(buf.cb, m_swapchainImage.image);
	VRAM::blit(buf.cb, {target.colour, m_swapchainImage});
	m_device->m_layouts.transition<lt::TransferPresent>(buf.cb, m_swapchainImage.image);
}
} // namespace le::graphics
