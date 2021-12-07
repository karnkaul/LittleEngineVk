#include <core/utils/expect.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/context.hpp>
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

Renderer::Cmd Renderer::Cmd::make(not_null<Device*> device, bool secondary) {
	Cmd ret;
	auto const level = secondary ? vk::CommandBufferLevel::eSecondary : vk::CommandBufferLevel::ePrimary;
	ret.pool = makeDeferred<vk::CommandPool>(device, vk::CommandPoolCreateFlagBits::eTransient, QType::eGraphics);
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

Deferred<vk::RenderPass> Renderer::makeRenderPass(not_null<Device*> device, Attachment colour, Attachment depth, Span<vk::SubpassDependency const> deps) {
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
	return {device, device->device().createRenderPass(createInfo)};
}

Deferred<vk::RenderPass> Renderer::makeSingleRenderPass(not_null<Device*> device, vk::Format colour, vk::Format depth, Span<vk::SubpassDependency const> deps) {
	Renderer::Attachment ac, ad;
	ac.format = colour;
	ad.format = depth;
	ac.layouts = {vIL::eColorAttachmentOptimal, vIL::eColorAttachmentOptimal};
	ad.layouts = {vIL::eDepthStencilAttachmentOptimal, vIL::eDepthStencilAttachmentOptimal};
	return Renderer::makeRenderPass(device, ac, ad, deps);
}

void Renderer::execRenderPass(not_null<Device*> device, IDrawer& out_drawer, PipelineFactory& pf, RenderInfo info) {
	EXPECT(info.primary.recording() && !info.primary.rendering());
	EXPECT(!info.secondary.empty() && info.framebuffer.colour.image && info.pass);
	if (!info.primary.recording() || info.secondary.empty() || !info.framebuffer.colour.image || !info.pass) { return; }
	bool const hasDepth = info.framebuffer.depth.image != vk::Image();
	vk::ImageView const colourDepth[] = {info.framebuffer.colour.view, info.framebuffer.depth.view};
	auto const views = hasDepth ? Span(colourDepth) : info.framebuffer.colour.view;
	auto const extent = info.framebuffer.extent();
	auto fb = Deferred<vk::Framebuffer>(device, device->makeFramebuffer(info.pass, views, graphics::cast(extent), 1U));
	vk::CommandBufferInheritanceInfo inh;
	inh.renderPass = info.pass;
	inh.framebuffer = fb;
	for (auto& cmd : info.secondary) { cmd.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inh); }
	info.secondary[0].setViewport(viewport(extent, info.begin.view));
	info.secondary[0].setScissor(scissor(extent, info.begin.view));
	out_drawer.beginPass(pf, info.pass);
	out_drawer.draw(info.secondary);
	for (auto& cmd : info.secondary) { cmd.end(); }
	auto const cc = info.begin.clear.toVec4();
	vk::ClearColorValue const clear = std::array{cc.x, cc.y, cc.z, cc.w};
	device->m_layouts.transition(info.primary, info.framebuffer.colour.image, vIL::eColorAttachmentOptimal, LayoutStages::topColour());
	if (hasDepth) { device->m_layouts.transition(info.primary, info.framebuffer.depth.image, vIL::eDepthStencilAttachmentOptimal, LayoutStages::topDepth()); }
	graphics::CommandBuffer::PassInfo passInfo;
	passInfo.clearValues = {clear, info.begin.depth};
	passInfo.subpassContents = vk::SubpassContents::eSecondaryCommandBuffers;
	passInfo.usage = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	info.primary.beginRenderPass(info.pass, fb, extent, passInfo);
	ktl::fixed_vector<vk::CommandBuffer, max_secondary_cmd_v> secondary;
	for (auto const& cmd : info.secondary) { secondary.push_back(cmd.m_cb); }
	info.primary.m_cb.executeCommands((u32)secondary.size(), secondary.data());
	info.primary.endRenderPass();
}

Renderer::Renderer(CreateInfo const& info) : m_depthImage(info.vram), m_colourImage(info.vram), m_vram(info.vram), m_target(info.target) {
	Buffering const buffering = info.buffering < 2_B ? 2_B : info.buffering;
	EXPECT(info.secondaryCmds <= max_secondary_cmd_v);
	EXPECT(info.target == Target::eSwapchain || info.surfaceBlitFlags.test(BlitFlag::eDst));
	if (!info.surfaceBlitFlags.test(BlitFlag::eDst)) { m_target = Target::eSwapchain; }
	u8 const secondaryCmds = std::clamp(info.secondaryCmds, u8(1), max_secondary_cmd_v);
	for (Buffering i{0}; i < buffering; ++i.value) {
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

Deferred<vk::RenderPass> Renderer::makeRenderPass(vk::Format colour, std::optional<vk::Format> depth, Span<vk::SubpassDependency const> deps) const {
	Attachment ac, ad;
	if (colour == vk::Format()) { colour = m_surfaceFormat.colour.format; }
	if (depth && *depth == vk::Format()) { depth = m_surfaceFormat.depth; }
	ac.format = colour;
	ad.format = depth.value_or(vk::Format());
	ac.layouts = {vIL::eColorAttachmentOptimal, vIL::eColorAttachmentOptimal};
	ad.layouts = {vIL::eDepthStencilAttachmentOptimal, vIL::eDepthStencilAttachmentOptimal};
	return makeRenderPass(m_vram->m_device, ac, ad, deps);
}

vk::CommandBuffer Renderer::render(IDrawer& out_drawer, PipelineFactory& pf, RenderTarget const& acquired, RenderBegin const& rb) {
	out_drawer.beginFrame();
	m_vram->m_device->resetCommandPool(m_primaryCmd.get().pool);
	for (auto& cmd : m_secondaryCmds.get()) { m_vram->m_device->resetCommandPool(cmd.pool); }
	doRender(out_drawer, pf, acquired, rb);
	auto ret = m_primaryCmd.get().cb.m_cb;
	next();
	return ret;
}

bool Renderer::canScale() const noexcept { return tech().target == Target::eOffScreen; }

bool Renderer::renderScale(f32 rs) noexcept {
	if (canScale() && rs >= m_scaleLimits.first && rs <= m_scaleLimits.second) {
		m_scale = rs;
		return true;
	}
	return false;
}

void Renderer::doRender(IDrawer& out_drawer, PipelineFactory& pf, RenderTarget const& acquired, RenderBegin const& rb) {
	auto& cmd = m_primaryCmd.get();
	Extent2D extent = acquired.extent;
	RenderTarget colour = acquired;
	if (m_target == Target::eOffScreen) {
		extent = scaleExtent(extent, renderScale());
		auto& img = m_colourImage.refresh(extent, m_colourFormat);
		colour = {img.image(), img.view(), img.extent2D(), img.viewFormat()};
	}
	auto& depthImage = m_depthImage.refresh(extent, m_surfaceFormat.depth);
	cmd.cb.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	RenderInfo ri;
	ri.begin = rb;
	ri.primary = cmd.cb;
	for (auto const& cmd : m_secondaryCmds.get()) { ri.secondary.push_back(cmd.cb); }
	ri.pass = m_singleRenderPass;
	ri.framebuffer.colour = colour;
	ri.framebuffer.depth = {depthImage.image(), depthImage.view(), depthImage.extent2D(), depthImage.viewFormat()};
	execRenderPass(m_vram->m_device, out_drawer, pf, std::move(ri));
	utils::Transition ltcolour{m_vram->m_device, &cmd.cb, colour.image};
	utils::Transition ltacquired{m_vram->m_device, &cmd.cb, acquired.image};
	if (m_target == Target::eOffScreen) {
		ltcolour(vIL::eTransferSrcOptimal, LayoutStages::colourTransfer());
		ltacquired(vIL::eTransferDstOptimal, LayoutStages::colourTransfer());
		m_vram->blit(cmd.cb, {colour, acquired});
	}
	ltacquired(vIL::ePresentSrcKHR, LayoutStages::transferBottom());
	cmd.cb.end();
}

void Renderer::next() {
	m_primaryCmd.next();
	m_secondaryCmds.next();
}
} // namespace le::graphics
