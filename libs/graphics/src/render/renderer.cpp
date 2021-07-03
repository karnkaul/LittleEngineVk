#include <graphics/context/device.hpp>
#include <graphics/render/renderer.hpp>
#include <graphics/render/swapchain.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
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

ARenderer::ARenderer(not_null<Swapchain*> swapchain, Buffering buffering, Extent2D extent, vk::Format depthFormat)
	: m_swapchain(swapchain), m_device(swapchain->m_device), m_fence(m_device, buffering) {
	depthImage(depthFormat, extent);
}

std::optional<RenderImage> ARenderer::depthImage(vk::Format depthFormat, Extent2D extent) {
	if (m_depth && cast(m_depth->extent()) == extent) { return renderImage(*m_depth); }
	m_depth.reset();
	if (depthFormat != vk::Format() && Swapchain::valid(extent)) {
		Image::CreateInfo info;
		info.createInfo.format = depthFormat;
		info.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		info.createInfo.extent = vk::Extent3D(cast(extent), 1);
		info.createInfo.tiling = vk::ImageTiling::eOptimal;
		info.createInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
		if constexpr (levk_desktopOS) {
			info.preferred = vk::MemoryPropertyFlagBits::eLazilyAllocated;
			info.createInfo.usage |= vk::ImageUsageFlagBits::eTransientAttachment;
		}
		info.createInfo.samples = vk::SampleCountFlagBits::e1;
		info.createInfo.imageType = vk::ImageType::e2D;
		info.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		info.createInfo.mipLevels = 1;
		info.createInfo.arrayLayers = 1;
		info.queueFlags = QType::eGraphics;
		info.view.format = info.createInfo.format;
		info.view.aspects = vk::ImageAspectFlagBits::eDepth;
		m_depth = Image(m_swapchain->m_vram, info);
		return renderImage(*m_depth);
	}
	return std::nullopt;
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
		ensure(hasDepthImage(), "No depth image in this Renderer instance");
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

vk::Framebuffer ARenderer::makeFramebuffer(vk::RenderPass renderPass, vAP<vk::ImageView> attachments, Extent2D extent, u32 layers) const {
	return m_device->makeFramebuffer(renderPass, attachments, cast(extent), layers);
}

void ARenderer::waitForFrame() {
	m_swapchain->m_vram->update(); // poll transfer if needed
	m_fence.wait();
	m_device->decrementDeferred(); // update deferred
}

bool ARenderer::renderScale(f32 rs) noexcept {
	if (tech().target == Target::eOffScreen) {
		m_scale = rs;
		return true;
	}
	return false;
}
} // namespace le::graphics
