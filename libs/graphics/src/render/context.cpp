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

Pipeline RenderContext::makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo info) {
	if (info.renderPass == vk::RenderPass()) { info.renderPass = m_renderer->renderPass(); }
	info.buffering = m_buffering;
	info.cache = m_pipelineCache;
	return Pipeline(m_vram, shader, std::move(info), id);
}

void RenderContext::waitForFrame() { m_vram->m_device->waitFor(m_syncs.get().drawn); }

bool RenderContext::render(IDrawer& out_drawer, RenderBegin const& rb, Extent2D fbSize) {
	if (fbSize.x == 0 || fbSize.y == 0) { return false; }
	bool ret{};
	auto& sync = m_syncs.get();
	if (auto acquired = m_surface.acquireNextImage(fbSize, sync.draw)) {
		m_vram->m_device->resetFence(sync.drawn);
		auto cmds = m_renderer->render(out_drawer, acquired->image, rb);
		ret = submit(cmds, *acquired, fbSize);
	}
	m_syncs.next();
	return ret;
}

bool RenderContext::recreateSwapchain(Extent2D fbSize, std::optional<VSync> vsync) {
	return m_surface.makeSwapchain(fbSize, vsync.value_or(m_surface.format().vsync));
}

bool RenderContext::submit(Span<vk::CommandBuffer const> cbs, Acquire const& acquired, Extent2D fbSize) {
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
	return m_surface.present(fbSize, acquired, sync.present);
}
} // namespace le::graphics
