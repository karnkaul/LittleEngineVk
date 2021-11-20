#include <core/log.hpp>
#include <core/maths.hpp>
#include <core/utils/expect.hpp>
#include <glm/gtx/transform.hpp>
#include <graphics/common.hpp>
#include <graphics/context/defer_queue.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/context.hpp>
#include <graphics/utils/utils.hpp>
#include <map>
#include <stdexcept>

namespace le::graphics {
namespace {
void validateBuffering([[maybe_unused]] Buffering images, Buffering buffering) {
	ENSURE(images > 1_B, "Insufficient swapchain images");
	ENSURE(buffering > 0_B, "Insufficient buffering");
	if ((s16)buffering.value - (s16)images.value > 1) { g_log.log(lvl::warn, 0, "[{}] Buffering significantly more than swapchain image count", g_name); }
	if (buffering < 2_B) { g_log.log(lvl::warn, 0, "[{}] Buffering less than double; expect hitches", g_name); }
}

std::unique_ptr<Renderer> makeRenderer(VRAM* vram, Surface::Format const& format, Buffering buffering) {
	Renderer::CreateInfo rci;
	rci.vram = vram;
	rci.format = format;
	rci.buffering = buffering;
	rci.target = Renderer::Target::eOffScreen;
	return std::make_unique<Renderer>(rci);
}

constexpr Extent2D scaleExtent(Extent2D extent, f32 scale) noexcept {
	glm::vec2 const ret = glm::vec2(f32(extent.x), f32(extent.y)) * scale;
	return {u32(ret.x), u32(ret.y)};
}
} // namespace

VertexInputInfo RenderContext::vertexInput(VertexInputCreateInfo const& info) {
	VertexInputInfo ret;
	u32 bindDelta = 0, locationDelta = 0;
	for (auto& type : info.types) {
		vk::VertexInputBindingDescription binding;
		binding.binding = u32(info.bindStart + bindDelta);
		binding.stride = (u32)type.size;
		binding.inputRate = type.inputRate;
		ret.bindings.push_back(binding);
		for (auto const& member : type.members) {
			vk::VertexInputAttributeDescription attribute;
			attribute.binding = u32(info.bindStart + bindDelta);
			attribute.format = member.format;
			attribute.offset = (u32)member.offset;
			attribute.location = u32(info.locationStart + locationDelta++);
			ret.attributes.push_back(attribute);
		}
		++bindDelta;
	}
	return ret;
}

VertexInputInfo RenderContext::vertexInput(QuickVertexInput const& info) {
	VertexInputInfo ret;
	vk::VertexInputBindingDescription binding;
	binding.binding = info.binding;
	binding.stride = (u32)info.size;
	binding.inputRate = vk::VertexInputRate::eVertex;
	ret.bindings.push_back(binding);
	u32 location = 0;
	for (auto const& [format, offset] : info.attributes) {
		vk::VertexInputAttributeDescription attribute;
		attribute.binding = info.binding;
		attribute.format = format;
		attribute.offset = (u32)offset;
		attribute.location = location++;
		ret.attributes.push_back(attribute);
	}
	return ret;
}

RenderContext::RenderContext(not_null<VRAM*> vram, Swapchain::CreateInfo const& info, glm::vec2 fbSize)
	: m_swapchain(vram, info, fbSize), m_pool(vram->m_device, vk::CommandPoolCreateFlagBits::eTransient), m_device(vram->m_device) {
	m_storage.pipelineCache = makeDeferred<vk::PipelineCache>(m_device);
}

void RenderContext::initRenderer() {
	m_storage.status = Status::eWaiting;
	validateBuffering(m_swapchain.buffering(), m_storage.renderer->buffering());
	DeferQueue::defaultDefer = m_storage.renderer->buffering();
}

Pipeline RenderContext::makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo info) {
	if (info.renderPass == vk::RenderPass()) { info.renderPass = m_storage.renderer->renderPass3D(); }
	info.buffering = m_storage.renderer->buffering();
	info.cache = m_storage.pipelineCache;
	return Pipeline(m_swapchain.m_vram, shader, std::move(info), id);
}

bool RenderContext::ready(glm::ivec2 framebufferSize) {
	if (m_storage.reconstruct.trigger || m_swapchain.flags().any(Swapchain::Flags(Swapchain::Flag::eOutOfDate) | Swapchain::Flag::ePaused)) {
		auto const& vsync = m_storage.reconstruct.vsync;
		bool const ret = vsync ? m_swapchain.reconstruct(*vsync, framebufferSize) : m_swapchain.reconstruct(framebufferSize);
		if (ret) {
			m_storage.renderer->refresh();
			m_storage.status = Status::eWaiting;
			m_storage.reconstruct = {};
		}
		return false;
	}
	return true;
}

bool RenderContext::waitForFrame() {
	if (!check(Status::eReady)) {
		if (m_storage.status != Status::eWaiting && m_storage.status != Status::eBegun) {
			g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
			return false;
		}
		m_storage.renderer->waitForFrame();
		m_pool.update();
		set(Status::eReady);
	}
	return true;
}

std::optional<RenderTarget> RenderContext::beginFrame() {
	if (!check(Status::eReady)) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return std::nullopt;
	}
	if (m_swapchain.flags().any(Swapchain::Flags(Swapchain::Flag::ePaused) | Swapchain::Flag::eOutOfDate)) { return std::nullopt; }
	if (auto ret = m_storage.renderer->beginFrame()) {
		set(Status::eBegun);
		m_storage.drawing = *ret;
		return ret;
	}
	return std::nullopt;
}

std::optional<CommandBuffer> RenderContext::beginDraw(ScreenView const& view, RGBA clear, ClearDepth depth) {
	if (!check(Status::eBegun) || !m_storage.drawing) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return std::nullopt;
	}
	set(Status::eDrawing);
	return m_storage.renderer->beginDraw(*m_storage.drawing, view, clear, depth);
}

bool RenderContext::endDraw() {
	if (!m_storage.drawing || !check(Status::eDrawing)) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	m_storage.renderer->endDraw(*m_storage.drawing);
	return true;
}

bool RenderContext::endFrame() {
	if (!check(Status::eDrawing, Status::eBegun)) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	set(Status::eEnded);
	m_storage.drawing.reset();
	m_storage.renderer->endFrame();
	return true;
}

bool RenderContext::submitFrame() {
	if (!check(Status::eEnded)) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	set(Status::eWaiting);
	if (m_storage.renderer->submitFrame()) { return true; }
	return false;
}

void RenderContext::reconstruct(std::optional<graphics::Vsync> vsync) {
	m_storage.reconstruct.trigger = true;
	m_storage.reconstruct.vsync = vsync;
}

glm::mat4 RenderContext::preRotate() const noexcept {
	glm::mat4 ret(1.0f);
	f32 rad = 0.0f;
	auto const transform = m_swapchain.display().transform;
	if (transform == vk::SurfaceTransformFlagBitsKHR::eIdentity) {
		return ret;
	} else if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate90) {
		rad = glm::radians(90.0f);
	} else if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate180) {
		rad = glm::radians(180.0f);
	} else if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate180) {
		rad = glm::radians(270.0f);
	}
	return glm::rotate(ret, rad, front);
}

vk::Viewport RenderContext::viewport(Extent2D extent, ScreenView const& view, glm::vec2 depth) const noexcept {
	return m_storage.renderer->viewport(Swapchain::valid(extent) ? extent : this->extent(), view, depth);
}

vk::Rect2D RenderContext::scissor(Extent2D extent, ScreenView const& view) const noexcept {
	return m_storage.renderer->scissor(Swapchain::valid(extent) ? extent : this->extent(), view);
}

// NEW

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
	m_info.view.format = m_info.createInfo.format;
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
	m_info.view.format = m_info.createInfo.format;
	m_info.view.aspects = vk::ImageAspectFlagBits::eColor;
	return m_info;
}

bool ImageCache::ready(Extent2D extent, vk::Format format) const noexcept {
	return m_image && cast(m_image->extent()) == extent && m_info.createInfo.format == format;
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
	m_renderPass3D = makeRenderPass(m_transition, colourFormat, m_surfaceFormat.depth);
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
	return foo::RenderContext::makeRenderPass(*m_vram->m_device, ac, ad, {});
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
		extent = scaleExtent(acquired.extent, renderScale());
		auto& img = m_colourImage.refresh(extent, m_colourFormat);
		colour = {img.image(), img.view(), cast(img.extent())};
	}
	auto& depth = m_depthImage.refresh(extent, m_surfaceFormat.depth);
	vk::ImageView const views[] = {colour.view, depth.view()};
	auto fb = makeFramebuffer(m_renderPass3D, views, extent);
	auto const cc = rb.clear.toVec4();
	vk::ClearColorValue const clear = std::array{cc.x, cc.y, cc.z, cc.w};
	graphics::CommandBuffer::PassInfo const passInfo{{clear, {}}, vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
	if (m_transition == Transition::eCommandBuffer) {
		m_vram->m_device->m_layouts.transition<lt::ColourWrite>(cmd.cb, colour.image);
		m_vram->m_device->m_layouts.transition<lt::DepthStencilWrite>(cmd.cb, depth.image(), depthStencil);
	}
	cmd.cb.beginRenderPass(m_renderPass3D, fb, acquired.extent, passInfo);
	cmd.cb.setViewport(viewport(acquired.extent, {}, {}));
	cmd.cb.setScissor(scissor(acquired.extent, {}));
	out_drawer.draw3D(cmd.cb);
	out_drawer.drawUI(cmd.cb);
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

namespace foo {
VertexInputInfo RenderContext::vertexInput(VertexInputCreateInfo const& info) {
	VertexInputInfo ret;
	u32 bindDelta = 0, locationDelta = 0;
	for (auto& type : info.types) {
		vk::VertexInputBindingDescription binding;
		binding.binding = u32(info.bindStart + bindDelta);
		binding.stride = (u32)type.size;
		binding.inputRate = type.inputRate;
		ret.bindings.push_back(binding);
		for (auto const& member : type.members) {
			vk::VertexInputAttributeDescription attribute;
			attribute.binding = u32(info.bindStart + bindDelta);
			attribute.format = member.format;
			attribute.offset = (u32)member.offset;
			attribute.location = u32(info.locationStart + locationDelta++);
			ret.attributes.push_back(attribute);
		}
		++bindDelta;
	}
	return ret;
}

VertexInputInfo RenderContext::vertexInput(QuickVertexInput const& info) {
	VertexInputInfo ret;
	vk::VertexInputBindingDescription binding;
	binding.binding = info.binding;
	binding.stride = (u32)info.size;
	binding.inputRate = vk::VertexInputRate::eVertex;
	ret.bindings.push_back(binding);
	u32 location = 0;
	for (auto const& [format, offset] : info.attributes) {
		vk::VertexInputAttributeDescription attribute;
		attribute.binding = info.binding;
		attribute.format = format;
		attribute.offset = (u32)offset;
		attribute.location = location++;
		ret.attributes.push_back(attribute);
	}
	return ret;
}

Deferred<vk::RenderPass> RenderContext::makeRenderPass(Device& device, Attachment colour, Attachment depth, vAP<vk::SubpassDependency> deps) {
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
	createInfo.dependencyCount = deps.size();
	createInfo.pDependencies = deps.data();
	return {&device, device.device().createRenderPass(createInfo)};
}

RenderContext::RenderContext(not_null<VRAM*> vram, std::optional<VSync> vsync, Extent2D fbSize, Buffering buffering)
	: m_surface(vram, fbSize, vsync), m_vram(vram), m_renderer(makeRenderer(m_vram, m_surface.format(), buffering)), m_buffering(buffering) {
	m_pipelineCache = makeDeferred<vk::PipelineCache>(m_vram->m_device);
	validateBuffering({(u8)m_surface.imageCount()}, m_buffering);
	DeferQueue::defaultDefer = m_buffering;
	for (Buffering i = {}; i < m_buffering; ++i.value) { m_syncs.push(Sync::make(m_vram->m_device)); }
}

std::unique_ptr<Renderer> RenderContext::defaultRenderer() { return makeRenderer(m_vram, m_surface.format(), m_buffering); }

void RenderContext::render(IDrawer& out_drawer, RenderBegin const& rb, Extent2D fbSize) {
	auto& sync = m_syncs.get();
	if (auto acquired = m_surface.acquireNextImage(fbSize, sync.draw)) {
		m_vram->m_device->waitFor(sync.drawn);
		m_vram->m_device->resetFence(sync.drawn);
		auto ret = m_renderer->render(out_drawer, acquired->image, rb);
		submit(ret, *acquired, fbSize);
	}
	m_syncs.next();
}

bool RenderContext::recreateSwapchain(Extent2D fbSize, std::optional<VSync> vsync) {
	return m_surface.makeSwapchain(fbSize, vsync.value_or(m_surface.format().vsync));
}

void RenderContext::submit(Span<vk::CommandBuffer const> cbs, Acquire const& acquired, Extent2D fbSize) {
	auto const& sync = m_syncs.get();
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags const waitStages = vPSFB::eTopOfPipe;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &sync.draw.m_t;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = (u32)cbs.size();
	submitInfo.pCommandBuffers = cbs.data();
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &sync.present.m_t;
	m_surface.submit(cbs, {sync.draw, sync.present, sync.drawn});
	m_surface.present(fbSize, acquired, sync.present);
}
} // namespace foo
} // namespace le::graphics
