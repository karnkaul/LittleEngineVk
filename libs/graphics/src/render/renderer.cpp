#include <core/utils/expect.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/renderer.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
vk::SubpassDependency makeSubpassDependency(bool offscreen) {
	vk::SubpassDependency ret;
	ret.srcSubpass = VK_SUBPASS_EXTERNAL;
	ret.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	ret.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
	ret.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	if (offscreen) {
		ret.srcAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite;
		ret.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;
		ret.srcStageMask |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}
	ret.dstStageMask = ret.srcStageMask;
	return ret;
}
} // namespace

Image::CreateInfo& ImageCache::setDepth() {
	static constexpr auto usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment;
	auto const vmaUsage = m_vram->m_device->physicalDevice().supportsLazyAllocation() ? VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED : VMA_MEMORY_USAGE_GPU_ONLY;
	m_info = Image::info({}, usage, vk::ImageAspectFlagBits::eDepth, vmaUsage, {});
	return m_info;
}

Image::CreateInfo& ImageCache::setColour() {
	static constexpr auto usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	m_info = Image::info({}, usage, vk::ImageAspectFlagBits::eColor, VMA_MEMORY_USAGE_GPU_ONLY, {});
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

Renderer::Cmd Renderer::Cmd::make(not_null<Device*> device, bool secondary) {
	Cmd ret;
	auto const level = secondary ? vk::CommandBufferLevel::eSecondary : vk::CommandBufferLevel::ePrimary;
	ret.pool = ret.pool.make(device->queues().graphics().makeCommandPool(device->device(), vk::CommandPoolCreateFlagBits::eTransient), device);
	ret.cb = CommandBuffer(*device, ret.pool, level);
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

Defer<vk::RenderPass> Renderer::makeRenderPass(not_null<Device*> device, Attachment colour, Attachment depth, Span<vk::SubpassDependency const> deps) {
	vk::AttachmentDescription attachments[2];
	u32 attachmentCount = 1;
	vk::AttachmentReference colourAttachment, depthAttachment;
	attachments[0].format = colour.format;
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
		attachmentCount = 2;
	}
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachment;
	subpass.pDepthStencilAttachment = depth.format == vk::Format() ? nullptr : &depthAttachment;
	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = attachmentCount;
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1U;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = (u32)deps.size();
	createInfo.pDependencies = deps.data();
	return Defer<vk::RenderPass>::make(device->device().createRenderPass(createInfo), device);
}

Defer<vk::RenderPass> Renderer::makeMainRenderPass(not_null<Device*> device, vk::Format colour, vk::Format depth, Span<vk::SubpassDependency const> deps) {
	Renderer::Attachment ac, ad;
	ac.format = colour;
	ad.format = depth;
	ac.layouts = {vIL::eColorAttachmentOptimal, vIL::eColorAttachmentOptimal};
	ad.layouts = {vIL::eDepthStencilAttachmentOptimal, vIL::eDepthStencilAttachmentOptimal};
	return Renderer::makeRenderPass(device, ac, ad, deps);
}

Renderer::Renderer(CreateInfo const& info) : m_depthImage(info.vram), m_colourImage(info.vram), m_vram(info.vram), m_target(info.target) {
	Buffering const buffering = info.buffering < Buffering::eDouble ? Buffering::eDouble : info.buffering;
	EXPECT(info.secondaryCmds <= max_secondary_cmd_v);
	EXPECT(info.target == Target::eSwapchain || info.surfaceBlitFlags.test(BlitFlag::eDst));
	if (!info.surfaceBlitFlags.test(BlitFlag::eDst)) { m_target = Target::eSwapchain; }
	u8 const secondaryCmds = std::min(info.secondaryCmds, max_secondary_cmd_v);
	for (Buffering i{0}; i < buffering; ++i) {
		m_primaryCmd.push(Cmd::make(m_vram->m_device));
		Cmds cmds;
		for (u8 j = 0; j < secondaryCmds; ++j) { cmds.push_back(Cmd::make(m_vram->m_device, true)); }
		m_secondaryCmds.push(std::move(cmds));
	}
	m_surfaceFormat = info.surfaceFormat;
	m_blitFlags = info.surfaceBlitFlags;
	auto const colourFormat = m_target == Target::eOffScreen ? m_colourFormat : m_surfaceFormat.colour.format;
	m_singleRenderPass = makeRenderPass(colourFormat, m_surfaceFormat.depth, makeSubpassDependency(m_target == Target::eOffScreen));
	m_depthImage.setDepth();
	m_colourImage.setColour();
}

Defer<vk::RenderPass> Renderer::makeRenderPass(vk::Format colour, std::optional<vk::Format> depth, Span<vk::SubpassDependency const> deps) const {
	Attachment ac, ad;
	if (colour == vk::Format()) { colour = m_surfaceFormat.colour.format; }
	if (depth && *depth == vk::Format()) { depth = m_surfaceFormat.depth; }
	ac.format = colour;
	ad.format = depth.value_or(vk::Format());
	ac.layouts = {vIL::eColorAttachmentOptimal, vIL::eColorAttachmentOptimal};
	ad.layouts = {vIL::eDepthStencilAttachmentOptimal, vIL::eDepthStencilAttachmentOptimal};
	return makeRenderPass(m_vram->m_device, ac, ad, deps);
}

RenderInfo Renderer::mainPassInfo(RenderTarget const& colour, RenderTarget const& depth, RenderBegin const& rb) const {
	RenderInfo ri;
	ri.begin = rb;
	ri.primary = m_primaryCmd.get().cb;
	for (auto const& cmd : m_secondaryCmds.get()) { ri.secondary.push_back(cmd.cb); }
	ri.renderPass = m_singleRenderPass;
	ri.framebuffer.colour = colour;
	ri.framebuffer.depth = depth;
	return ri;
}

RenderPass Renderer::beginMainPass(PipelineFactory& pf, RenderTarget const& acquired, RenderBegin const& rb) {
	Extent2D extent = acquired.extent;
	RenderTarget colour = acquired;
	m_acquired = acquired;
	if (m_target == Target::eOffScreen) {
		extent = scaleExtent(extent, renderScale());
		auto& img = m_colourImage.refresh(extent, m_colourFormat);
		colour = RenderTarget{img.ref()};
	}
	auto& depthImage = m_depthImage.refresh(extent, m_surfaceFormat.depth);
	auto const depth = RenderTarget{depthImage.ref()};
	auto& cmd = m_primaryCmd.get();
	m_vram->m_device->resetCommandPool(cmd.pool);
	for (auto& cmd : m_secondaryCmds.get()) { m_vram->m_device->resetCommandPool(cmd.pool); }
	cmd.cb.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	return RenderPass(m_vram->m_device, &pf, mainPassInfo(colour, depth, rb));
}

vk::CommandBuffer Renderer::endMainPass(RenderPass& out_rp) {
	auto& cmd = m_primaryCmd.get();
	out_rp.end();
	auto const colour = out_rp.info().framebuffer.colour;
	utils::Transition ltcolour{m_vram->m_device, &cmd.cb, colour.image};
	utils::Transition ltacquired{m_vram->m_device, &cmd.cb, m_acquired.image};
	LayoutStages presentLS = {LayoutStages::sa_colour_v, LayoutStages::sa_bottom_v};
	if (m_target == Target::eOffScreen) {
		ltcolour(vIL::eTransferSrcOptimal, {}, LayoutStages::colourTransfer());
		ltacquired(vIL::eTransferDstOptimal, {}, LayoutStages::colourTransfer());
		m_vram->blit(cmd.cb, {colour.ref(), m_acquired.ref()});
		presentLS.src = LayoutStages::sa_transfer_v;
	}
	ltacquired(vIL::ePresentSrcKHR, {}, presentLS);
	cmd.cb.end();
	m_acquired = {};
	next();
	return cmd.cb.m_cb;
}

bool Renderer::canScale() const noexcept { return tech().target == Target::eOffScreen; }

bool Renderer::renderScale(f32 rs) noexcept {
	if (canScale() && rs >= m_scaleLimits.first && rs <= m_scaleLimits.second) {
		m_scale = rs;
		return true;
	}
	return false;
}

void Renderer::next() {
	m_primaryCmd.next();
	m_secondaryCmds.next();
}
} // namespace le::graphics
