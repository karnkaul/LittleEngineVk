#include <graphics/context/device.hpp>
#include <graphics/render/renderers.hpp>

namespace le::graphics {
RendererFSR::RendererFSR(not_null<Swapchain*> swapchain, Buffering buffering) : ARenderer(swapchain, buffering) { m_storage = make(tech_v.transition); }

std::optional<ARenderer::Draw> RendererFSR::beginFrame() {
	if (auto acq = acquire()) {
		RenderTarget const target{acq->image, depthImage(acq->image.extent)};
		return Draw{target, m_storage.buf.get().cb};
	}
	return std::nullopt;
}

void RendererFSR::endDraw(RenderTarget const& target) {
	auto& buf = m_storage.buf.get();
	buf.cb.endRenderPass();
	m_device->m_layouts.drawn(target.depth.image);
}

RendererFSC::RendererFSC(not_null<Swapchain*> swapchain, Buffering buffering) : ARenderer(swapchain, buffering) { m_storage = make(tech_v.transition); }

std::optional<RendererFSC::Draw> RendererFSC::beginFrame() {
	if (auto acq = acquire()) {
		RenderTarget const target{acq->image, depthImage(acq->image.extent)};
		return Draw{target, m_storage.buf.get().cb};
	}
	return std::nullopt;
}

void RendererFSC::endDraw(RenderTarget const& target) {
	auto& buf = m_storage.buf.get();
	buf.cb.endRenderPass();
	m_device->m_layouts.transition<lt::TransferPresent>(buf.cb, target.colour.image);
	m_device->m_layouts.drawn(target.depth.image);
}

RendererFOC::RendererFOC(not_null<Swapchain*> swapchain, Buffering buffering) : ARenderer(swapchain, buffering) {
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
	renderScale(0.75f);
}

std::optional<RendererFOC::Draw> RendererFOC::beginFrame() {
	if (auto acq = acquire()) {
		auto& buf = m_storage.buf.get();
		m_swapchainImage = acq->image;
		auto const extent = scaleExtent(m_swapchainImage.extent, renderScale());
		auto depth = depthImage(extent);
		RenderTarget const target{renderImage(m_imageMaker.refresh(buf.offscreen, extent, m_colourFormat, m_colourIndex)), depth};
		return Draw{target, buf.cb};
	}
	return std::nullopt;
}

void RendererFOC::endDraw(RenderTarget const& target) {
	auto& buf = m_storage.buf.get();
	buf.cb.endRenderPass();
	m_device->m_layouts.transition<lt::TransferSrc>(buf.cb, target.colour.image);
	m_device->m_layouts.transition<lt::TransferDst>(buf.cb, m_swapchainImage.image);
	vk::Extent3D off = vk::Extent3D{cast(target.colour.extent), 1};
	vk::Extent3D swp = vk::Extent3D{cast(m_swapchainImage.extent), 1};
	m_swapchain->m_vram->blit(buf.cb.m_cb, target.colour.image, m_swapchainImage.image, {off, swp});
	m_device->m_layouts.transition<lt::TransferPresent>(buf.cb, m_swapchainImage.image);
	m_device->m_layouts.drawn(target.depth.image);
}
} // namespace le::graphics
