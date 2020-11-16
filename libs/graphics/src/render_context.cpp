#include <map>
#include <stdexcept>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <core/utils.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render_context.hpp>

namespace le::graphics {
RenderContext::Render::Render(RenderContext& context, Frame&& frame) : m_frame(std::move(frame)), m_context(context) {
}

RenderContext::Render::Render(Render&& rhs) : m_frame(std::exchange(rhs.m_frame, Frame())), m_context(rhs.m_context) {
}

RenderContext::Render& RenderContext::Render::operator=(Render&& rhs) {
	if (&rhs != this) {
		destroy();
		m_frame = std::exchange(rhs.m_frame, Frame());
		m_context = rhs.m_context;
	}
	return *this;
}

RenderContext::Render::~Render() {
	destroy();
}

void RenderContext::Render::destroy() {
	if (!default_v(m_frame.primary.m_cmd)) {
		RenderContext& rc = m_context;
		if (!rc.endFrame()) {
			logD("[{}] RenderContext failed to end frame", g_name);
		}
	}
}

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
	static std::vector<std::size_t> const single = {0};
	for (auto const& offset : (info.offsets.empty() ? single : info.offsets)) {
		vk::VertexInputAttributeDescription attribute;
		attribute.binding = info.binding;
		attribute.format = vk::Format::eR32G32B32Sfloat;
		attribute.offset = (u32)offset;
		attribute.location = location++;
		ret.attributes.push_back(attribute);
	}
	return ret;
}

vk::SamplerCreateInfo RenderContext::samplerInfo(MinMag minMag, vk::SamplerMipmapMode mip) {
	vk::SamplerCreateInfo ret;
	ret.magFilter = minMag.first;
	ret.minFilter = minMag.second;
	ret.addressModeU = ret.addressModeV = ret.addressModeW = vk::SamplerAddressMode::eRepeat;
	ret.borderColor = vk::BorderColor::eIntOpaqueBlack;
	ret.unnormalizedCoordinates = false;
	ret.compareEnable = false;
	ret.compareOp = vk::CompareOp::eAlways;
	ret.mipmapMode = mip;
	ret.mipLodBias = 0.0f;
	ret.minLod = 0.0f;
	ret.maxLod = 0.0f;
	return ret;
}

RenderContext::RenderContext(Swapchain& swapchain, u32 rotateCount, u32 secondaryCmdCount)
	: m_sync(swapchain.m_device, rotateCount, secondaryCmdCount), m_swapchain(swapchain), m_vram(swapchain.m_vram), m_device(swapchain.m_device) {
	m_storage.status = Status::eReady;
}

RenderContext::RenderContext(RenderContext&& rhs)
	: m_sync(std::move(rhs.m_sync)), m_storage(std::move(rhs.m_storage)), m_swapchain(rhs.m_swapchain), m_vram(rhs.m_vram), m_device(rhs.m_device) {
	rhs.m_storage.status = Status::eIdle;
}

RenderContext& RenderContext::operator=(RenderContext&& rhs) {
	if (&rhs != this) {
		m_sync = std::move(rhs.m_sync);
		m_storage = std::move(rhs.m_storage);
		m_swapchain = rhs.m_swapchain;
		m_vram = rhs.m_vram;
		m_device = rhs.m_device;
		rhs.m_storage.status = Status::eIdle;
	}
	return *this;
}

RenderContext::~RenderContext() {
	destroy();
}

bool RenderContext::waitForFrame() {
	if (m_storage.status == Status::eReady) {
		return true;
	}
	if (m_storage.status != Status::eWaiting) {
		logW("[{}] Invalid RenderContext status", g_name);
		return false;
	}
	Device& d = m_device;
	auto& sync = m_sync.get();
	d.waitFor(sync.drawing);
	d.decrementDeferred();
	m_storage.status = Status::eReady;
	return true;
}

std::optional<RenderContext::Frame> RenderContext::beginFrame(CommandBuffer::PassInfo const& info) {
	if (m_storage.status != Status::eReady) {
		logW("[{}] Invalid RenderContext status", g_name);
		return std::nullopt;
	}
	Device& d = m_device;
	Swapchain& s = m_swapchain;
	if (s.flags().any(Swapchain::Flag::ePaused | Swapchain::Flag::eOutOfDate)) {
		return std::nullopt;
	}
	FrameSync& sync = m_sync.get();
	auto target = s.acquireNextImage(sync.drawReady);
	if (!target) {
		return std::nullopt;
	}
	m_storage.status = Status::eDrawing;
	d.destroy(sync.framebuffer);
	sync.framebuffer = d.createFramebuffer(s.renderPass(), target->attachments(), target->extent);
	if (!sync.primary.commandBuffer.begin(s.renderPass(), sync.framebuffer, target->extent, info)) {
		ENSURE(false, "Failed to begin recording command buffer");
		d.destroy(sync.framebuffer, sync.drawReady);
		sync.drawReady = d.createSemaphore(); // sync.drawReady will be signalled by acquireNextImage and cannot be reused
		return std::nullopt;
	}
	return Frame{*target, sync.primary.commandBuffer};
}

std::optional<RenderContext::Render> RenderContext::render(Colour clear, vk::ClearDepthStencilValue depth) {
	vk::ClearColorValue const c = std::array{clear.r.toF32(), clear.g.toF32(), clear.b.toF32(), clear.a.toF32()};
	CommandBuffer::PassInfo const pass{{c, depth}};
	if (auto frame = beginFrame(pass)) {
		return Render(*this, std::move(*frame));
	}
	return std::nullopt;
}

bool RenderContext::endFrame() {
	if (m_storage.status != Status::eDrawing) {
		logW("[{}] Invalid RenderContext status", g_name);
		return false;
	}
	Device& d = m_device;
	Swapchain& s = m_swapchain;
	FrameSync& sync = m_sync.get();
	sync.primary.commandBuffer.end();
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags const waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &sync.drawReady;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &sync.primary.commandBuffer.m_cmd;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &sync.presentReady;
	d.m_device.resetFences(sync.drawing);
	d.m_queues.submit(QType::eGraphics, submitInfo, sync.drawing);
	m_storage.status = Status::eWaiting;
	auto present = s.present(sync.presentReady, sync.drawing);
	if (!present) {
		return false;
	}
	m_sync.next();
	return true;
}

bool RenderContext::reconstructd(glm::ivec2 framebufferSize) {
	if (!Swapchain::valid(framebufferSize)) {
		return false;
	}
	Swapchain& swapchain = m_swapchain;
	auto const flags = swapchain.flags();
	if (flags.any(Swapchain::Flag::eOutOfDate | Swapchain::Flag::eSuboptimal | Swapchain::Flag::ePaused)) {
		if (swapchain.reconstruct(framebufferSize)) {
			m_storage.status = Status::eWaiting;
			return true;
		}
	}
	return false;
}

View<Pipeline> RenderContext::makePipeline(std::string_view id, Pipeline::CreateInfo createInfo) {
	Swapchain& s = m_swapchain;
	createInfo.dynamicState.renderPass = s.renderPass();
	Pipeline p0(m_vram, std::move(createInfo), id);
	auto [iter, bResult] = m_storage.pipes.emplace(id, std::move(p0));
	if (!bResult || iter == m_storage.pipes.end()) {
		throw std::runtime_error("Map insertion failure");
	}
	logD("[{}] Pipeline [{}] constructed", g_name, id);
	return iter->second;
}

bool RenderContext::destroyPipeline(Hash id) {
	if (auto search = m_storage.pipes.find(id); search != m_storage.pipes.end()) {
		m_storage.pipes.erase(id);
		return true;
	}
	return false;
}

bool RenderContext::hasPipeline(Hash id) const noexcept {
	return m_storage.pipes.find(id) != m_storage.pipes.end();
}

View<Pipeline> RenderContext::pipeline(Hash id) {
	if (auto search = m_storage.pipes.find(id); search != m_storage.pipes.end()) {
		return search->second;
	}
	return {};
}

vk::Sampler RenderContext::makeSampler(vk::SamplerCreateInfo const& info) {
	Device& d = m_device;
	vk::Sampler ret = d.m_device.createSampler(info);
	m_storage.samplers.push_back(ret);
	return ret;
}

vk::Viewport RenderContext::viewport(vk::Extent2D extent, glm::vec2 const& depth, ScreenRect const& nRect) const noexcept {
	if (extent.width == 0 || extent.height == 0) {
		Swapchain& s = m_swapchain;
		extent = s.m_storage.extent;
	}
	vk::Viewport ret;
	glm::vec2 const size = nRect.size();
	ret.minDepth = depth.x;
	ret.maxDepth = depth.y;
	ret.width = size.x * (f32)extent.width;
	ret.height = -(size.y * (f32)extent.height); // flip viewport about X axis
	ret.x = nRect.lt.x * (f32)extent.width;
	ret.y = nRect.lt.y * (f32)extent.height - (f32)ret.height;
	return ret;
}

vk::Rect2D RenderContext::scissor(vk::Extent2D extent, ScreenRect const& nRect) const noexcept {
	if (extent.width == 0 || extent.height == 0) {
		Swapchain& s = m_swapchain;
		extent = s.m_storage.extent;
	}
	vk::Rect2D scissor;
	glm::vec2 const size = nRect.size();
	scissor.offset.x = (s32)(nRect.lt.x * (f32)extent.width);
	scissor.offset.y = (s32)(nRect.lt.y * (f32)extent.height);
	scissor.extent.width = (u32)(size.x * (f32)extent.width);
	scissor.extent.height = (u32)(size.y * (f32)extent.height);
	return scissor;
}

void RenderContext::destroy() {
	Device& d = m_device;
	d.defer([s = m_storage.samplers, &d]() mutable {
		for (auto sm : s) {
			d.destroy(sm);
		}
	});
	m_storage.pipes.clear();
}
} // namespace le::graphics
