#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/context.hpp>
#include <graphics/render/renderer.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
Image::CreateInfo& ImageCache::setDepth() {
	m_info = {};
	m_info.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	m_info.createInfo.tiling = vk::ImageTiling::eOptimal;
	m_info.createInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	m_info.preferred = vk::MemoryPropertyFlagBits::eLazilyAllocated;
	m_info.createInfo.usage |= vk::ImageUsageFlagBits::eTransientAttachment;
	m_info.createInfo.samples = vk::SampleCountFlagBits::e1;
	m_info.createInfo.imageType = vk::ImageType::e2D;
	m_info.createInfo.initialLayout = vk::ImageLayout::eUndefined;
	m_info.createInfo.mipLevels = 1;
	m_info.createInfo.arrayLayers = 1;
	m_info.queueFlags = QType::eGraphics;
	m_info.view.aspects = vk::ImageAspectFlagBits::eDepth;
	return m_info;
}

Image::CreateInfo& ImageCache::setColour() {
	m_info = {};
	m_info.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	m_info.createInfo.tiling = vk::ImageTiling::eOptimal;
	m_info.createInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	m_info.createInfo.samples = vk::SampleCountFlagBits::e1;
	m_info.createInfo.imageType = vk::ImageType::e2D;
	m_info.createInfo.initialLayout = vIL::eUndefined;
	m_info.createInfo.mipLevels = 1;
	m_info.createInfo.arrayLayers = 1;
	m_info.queueFlags = QFlags(QType::eTransfer) | QType::eGraphics;
	m_info.view.aspects = vk::ImageAspectFlagBits::eColor;
	return m_info;
}

bool ImageCache::ready(Extent2D extent, vk::Format format) const noexcept {
	return m_image && m_image->extent2D() == extent && m_info.createInfo.format == format;
}

Image& ImageCache::make(Extent2D extent, vk::Format format) {
	m_info.createInfo.extent = vk::Extent3D(extent.x, extent.y, 1);
	m_info.createInfo.format = m_info.view.format = format;
	m_image.emplace(m_vram, m_info);
	return *m_image;
}

Image& ImageCache::refresh(Extent2D extent, vk::Format format) {
	if (!ready(extent, format)) { make(extent, format); }
	return *m_image;
}

Renderer::Cmd Renderer::Cmd::make(not_null<Device*> device) {
	Cmd ret;
	ret.pool = makeDeferred<vk::CommandPool>(device, vk::CommandPoolCreateFlagBits::eTransient, QType::eGraphics);
	ret.cb = CommandBuffer::make(device, ret.pool, 1U).front();
	return ret;
}

vk::Viewport Renderer::viewport(Extent2D extent, ScreenView const& view, glm::vec2 depth) noexcept {
	DrawViewport ret;
	glm::vec2 const e(extent);
	ret.lt = view.nRect.lt * e + view.offset;
	ret.rb = view.nRect.rb * e + view.offset;
	ret.depth = depth;
	return utils::viewport(ret);
}

vk::Rect2D Renderer::scissor(Extent2D extent, ScreenView const& view) noexcept {
	DrawScissor ret;
	glm::vec2 const e(extent);
	ret.lt = view.nRect.lt * e + view.offset;
	ret.rb = view.nRect.rb * e + view.offset;
	return utils::scissor(ret);
}

Renderer::Renderer(CreateInfo const& info)
	: m_depthImage(info.vram), m_colourImage(info.vram), m_vram(info.vram), m_transition(info.transition), m_target(info.target) {
	if (m_target == Target::eOffScreen) { m_transition = Transition::eCommandBuffer; }
	Buffering const buffering = info.buffering < 2_B ? 2_B : info.buffering;
	u8 const cmdPerFrame = info.cmdPerFrame < 1 ? 1 : info.cmdPerFrame;
	for (Buffering i{0}; i < buffering; ++i.value) {
		Cmds cmds;
		for (u8 j = 0; j < cmdPerFrame; ++j) { cmds.push_back(Cmd::make(m_vram->m_device)); }
		m_cmds.push(std::move(cmds));
	}
	m_surfaceFormat = info.format;
	auto const colourFormat = m_target == Target::eOffScreen ? m_colourFormat : m_surfaceFormat.colour.format;
	m_singleRenderPass = makeRenderPass(m_transition, colourFormat, m_surfaceFormat.depth);
	m_depthImage.setDepth();
	m_colourImage.setColour();
}

Deferred<vk::Framebuffer> Renderer::makeFramebuffer(vk::RenderPass rp, Span<vk::ImageView const> views, Extent2D extent, u32 layers) const {
	return makeDeferred<vk::Framebuffer>(m_vram->m_device, rp, views, graphics::cast(extent), layers);
}

Deferred<vk::RenderPass> Renderer::makeRenderPass(Transition transition, vk::Format colour, std::optional<vk::Format> depth) const {
	Attachment ac, ad;
	if (colour == vk::Format()) { colour = m_surfaceFormat.colour.format; }
	if (depth && *depth == vk::Format()) { depth = m_surfaceFormat.depth; }
	ac.format = colour;
	ad.format = depth.value_or(vk::Format());
	if (transition == Transition::eRenderPass) {
		ac.layouts = {vIL::eUndefined, vIL::ePresentSrcKHR};
		ad.layouts = {vIL::eUndefined, vIL::eDepthStencilAttachmentOptimal};
	} else {
		ac.layouts = {vIL::eColorAttachmentOptimal, vIL::eColorAttachmentOptimal};
		ad.layouts = {vIL::eDepthStencilAttachmentOptimal, vIL::eDepthStencilAttachmentOptimal};
	}
	return RenderContext::makeRenderPass(*m_vram->m_device, ac, ad, {});
}

CmdBufs Renderer::render(IDrawer& out_drawer, RenderImage const& acquired, RenderBegin const& rb) {
	for (auto& cmd : m_cmds.get()) {
		m_vram->m_device->resetCommandPool(cmd.pool);
		cmd.cb.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	}
	doRender(out_drawer, acquired, rb);
	CmdBufs ret;
	for (auto& cmd : m_cmds.get()) {
		cmd.cb.end();
		ret.push_back(cmd.cb.m_cb);
	}
	next();
	return ret;
}

bool Renderer::canScale() const noexcept { return tech().target == Target::eOffScreen; }

bool Renderer::renderScale(f32 rs) noexcept {
	if (canScale()) {
		m_scale = rs;
		return true;
	}
	return false;
}

void Renderer::doRender(IDrawer& out_drawer, RenderImage const& acquired, RenderBegin const& rb) {
	auto& cmd = m_cmds.get().front();
	Extent2D extent = acquired.extent;
	RenderImage colour = acquired;
	if (m_target == Target::eOffScreen) {
		extent = scaleExtent(extent, renderScale());
		auto& img = m_colourImage.refresh(extent, m_colourFormat);
		colour = {img.image(), img.view(), img.extent2D()};
	}
	auto& depth = m_depthImage.refresh(extent, m_surfaceFormat.depth);
	vk::ImageView const views[] = {colour.view, depth.view()};
	auto fb = makeFramebuffer(m_singleRenderPass, views, extent);
	auto const cc = rb.clear.toVec4();
	vk::ClearColorValue const clear = std::array{cc.x, cc.y, cc.z, cc.w};
	graphics::CommandBuffer::PassInfo const passInfo{{clear, rb.depth}, vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
	if (m_transition == Transition::eCommandBuffer) {
		m_vram->m_device->m_layouts.transition<lt::ColourWrite>(cmd.cb, colour.image);
		m_vram->m_device->m_layouts.transition<lt::DepthStencilWrite>(cmd.cb, depth.image(), depthStencil);
	}
	cmd.cb.beginRenderPass(m_singleRenderPass, fb, extent, passInfo);
	cmd.cb.setViewport(viewport(extent, rb.view));
	cmd.cb.setScissor(scissor(extent, rb.view));
	out_drawer.draw(cmd.cb);
	cmd.cb.endRenderPass();
	if (m_transition == Transition::eCommandBuffer) {
		if (m_target == Target::eOffScreen) {
			m_vram->m_device->m_layouts.transition<lt::TransferSrc>(cmd.cb, colour.image);
			m_vram->m_device->m_layouts.transition<lt::TransferDst>(cmd.cb, acquired.image);
			VRAM::blit(cmd.cb, {colour, acquired});
		}
		m_vram->m_device->m_layouts.transition<lt::TransferPresent>(cmd.cb, acquired.image);
	}
}

void Renderer::next() { m_cmds.next(); }
} // namespace le::graphics
