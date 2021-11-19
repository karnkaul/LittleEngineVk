#include <graphics/context/device.hpp>
#include <graphics/render/renderer.hpp>
#include <graphics/render/swapchain.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
bool ImageMaker::ready(std::optional<Image>& out, Extent2D extent, vk::Format format, std::size_t idx) const noexcept {
	auto const& info = infos[idx];
	return out && cast(out->extent()) == extent && info.createInfo.format == format;
}

Image ImageMaker::make(Extent2D extent, vk::Format format, std::size_t idx) {
	auto& info = infos[idx];
	info.createInfo.extent = vk::Extent3D(extent.x, extent.y, 1);
	info.createInfo.format = info.view.format = format;
	return Image(vram, info);
}

Image& ImageMaker::refresh(std::optional<Image>& out, Extent2D extent, vk::Format format, std::size_t idx) {
	if (!ready(out, extent, format, idx)) { out = make(extent, format, idx); }
	return *out;
}

vk::Viewport ARenderer::viewport(Extent2D extent, ScreenView const& view, glm::vec2 depth) noexcept {
	DrawViewport ret;
	glm::vec2 const e(extent);
	ret.lt = view.nRect.lt * e + view.offset;
	ret.rb = view.nRect.rb * e + view.offset;
	ret.depth = depth;
	return utils::viewport(ret);
}

vk::Rect2D ARenderer::scissor(Extent2D extent, ScreenView const& view) noexcept {
	DrawScissor ret;
	glm::vec2 const e(extent);
	ret.lt = view.nRect.lt * e + view.offset;
	ret.rb = view.nRect.rb * e + view.offset;
	return utils::scissor(ret);
}

ARenderer::ARenderer(not_null<Swapchain*> swapchain, Buffering buffering)
	: m_swapchain(swapchain), m_device(swapchain->m_device), m_fence(m_device, buffering), m_imageMaker(m_swapchain->m_vram) {
	Image::CreateInfo depthInfo;
	depthInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	depthInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
	depthInfo.createInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	depthInfo.preferred = vk::MemoryPropertyFlagBits::eLazilyAllocated;
	depthInfo.createInfo.usage |= vk::ImageUsageFlagBits::eTransientAttachment;
	depthInfo.createInfo.samples = vk::SampleCountFlagBits::e1;
	depthInfo.createInfo.imageType = vk::ImageType::e2D;
	depthInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
	depthInfo.createInfo.mipLevels = 1;
	depthInfo.createInfo.arrayLayers = 1;
	depthInfo.queueFlags = QType::eGraphics;
	depthInfo.view.format = depthInfo.createInfo.format;
	depthInfo.view.aspects = vk::ImageAspectFlagBits::eDepth;
	m_imageMaker.infos.push_back(depthInfo);
	m_depthIndex = m_imageMaker.infos.size() - 1;
}

RenderImage ARenderer::depthImage(Extent2D extent, vk::Format format) {
	if (format == vk::Format()) { format = m_swapchain->depthFormat(); }
	return renderImage(m_imageMaker.refresh(m_depthImage, extent, format, m_depthIndex));
}

RenderSemaphore ARenderer::makeSemaphore() const { return {m_device->makeSemaphore(), m_device->makeSemaphore()}; }

vk::RenderPass ARenderer::makeRenderPass(Attachment colour, Attachment depth, vAP<vk::SubpassDependency> deps) const {
	std::array<vk::AttachmentDescription, 2> attachments;
	vk::AttachmentReference colourAttachment, depthAttachment;
	attachments[0].format = colour.format == vk::Format() ? m_swapchain->colourFormat().format : colour.format;
	attachments[0].samples = vk::SampleCountFlagBits::e1;
	attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
	attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
	attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[0].initialLayout = colour.layouts.first;
	attachments[0].finalLayout = colour.layouts.second;
	colourAttachment.attachment = 0;
	colourAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
	if (depth.format != vk::Format()) {
		attachments[1].format = depth.format;
		attachments[1].samples = vk::SampleCountFlagBits::e1;
		attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
		attachments[1].storeOp = vk::AttachmentStoreOp::eDontCare;
		attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eClear;
		attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachments[1].initialLayout = depth.layouts.first;
		attachments[1].finalLayout = depth.layouts.second;
		depthAttachment.attachment = 1;
		depthAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	}
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachment;
	subpass.pDepthStencilAttachment = depth.format == vk::Format() ? nullptr : &depthAttachment;
	return m_device->makeRenderPass(attachments, subpass, deps);
}

vk::Framebuffer ARenderer::makeFramebuffer(vk::RenderPass renderPass, Span<vk::ImageView const> attachments, Extent2D extent, u32 layers) const {
	return m_device->makeFramebuffer(renderPass, attachments, cast(extent), layers);
}

void ARenderer::waitForFrame() {
	m_swapchain->m_vram->update(); // poll transfer if needed
	m_fence.wait();
	m_device->decrementDeferred(); // update deferred
}

std::optional<RenderTarget> ARenderer::beginFrame() {
	if (auto acq = acquire()) { return RenderTarget{acq->image, depthImage(acq->image.extent)}; }
	return std::nullopt;
}

void ARenderer::endFrame() { m_storage.buf.get().cb.end(); }

bool ARenderer::submitFrame() {
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
	if (!m_fence.present(*m_swapchain, submitInfo, buf.present)) { return false; }
	m_storage.buf.next();
	return true;
}

bool ARenderer::canScale() const noexcept { return tech().target == Target::eOffScreen; }

bool ARenderer::renderScale(f32 rs) noexcept {
	if (canScale()) {
		m_scale = rs;
		return true;
	}
	return false;
}

ARenderer::Storage ARenderer::make(Transition transition, TPair<vk::Format> colourDepth) const {
	Storage ret;
	for (u8 i = 0; i < m_fence.buffering().value; ++i) { ret.buf.push({m_device, vk::CommandPoolCreateFlagBits::eTransient}); }
	Attachment colour, depth;
	if (colourDepth.first == vk::Format()) { colourDepth.first = m_swapchain->colourFormat().format; }
	if (colourDepth.second == vk::Format()) { colourDepth.second = m_swapchain->depthFormat(); }
	colour.format = colourDepth.first;
	depth.format = colourDepth.second;
	if (transition == Transition::eRenderPass) {
		colour.layouts = {vIL::eUndefined, vIL::ePresentSrcKHR};
		depth.layouts = {vIL::eUndefined, vIL::eDepthStencilAttachmentOptimal};
	} else {
		colour.layouts = {vIL::eColorAttachmentOptimal, vIL::eColorAttachmentOptimal};
		depth.layouts = {vIL::eDepthStencilAttachmentOptimal, vIL::eDepthStencilAttachmentOptimal};
	}
	ret.renderPass = {m_device, makeRenderPass(colour, depth, {})};
	return ret;
}

ktl::expected<Swapchain::Acquire, Swapchain::Flags> ARenderer::acquire(bool begin) {
	auto& buf = m_storage.buf.get();
	auto acquire = m_fence.acquire(*m_swapchain, buf.draw);
	if (acquire && begin) {
		m_device->device().resetCommandPool(buf.pool, {});
		buf.cb.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	}
	return acquire;
}
} // namespace le::graphics
