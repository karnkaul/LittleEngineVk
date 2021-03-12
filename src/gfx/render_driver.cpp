#include <algorithm>
#include <unordered_set>
#include <core/ensure.hpp>
#include <core/log.hpp>
#include <core/transform.hpp>
#include <engine/resources/resources.hpp>
#include <gfx/device.hpp>
#include <gfx/ext_gui.hpp>
#include <gfx/pipeline_impl.hpp>
#include <gfx/render_cmd.hpp>
#include <gfx/render_driver_impl.hpp>
#include <gfx/resource_descriptors.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <resources/resources_impl.hpp>
#include <window/window_impl.hpp>

namespace le::gfx::render {
namespace {
struct TexSet final {
	std::unordered_map<res::GUID, u32> idxMap;
	std::deque<res::Texture> textures;

	u32 add(res::Texture tex);
	u32 total() const;
};

u32 TexSet::add(res::Texture tex) {
	if (auto search = idxMap.find(tex.guid); search != idxMap.end()) {
		return search->second;
	}
	u32 const idx = (u32)textures.size();
	idxMap[tex.guid] = idx;
	textures.push_back(tex);
	return idx;
}

u32 TexSet::total() const {
	return (u32)textures.size();
}

template <typename... T>
bool allReady(T... t) {
	return ((t.status() == res::Status::eReady) && ...);
}
} // namespace

Driver::Driver() = default;
Driver::Driver(Driver&&) = default;
Driver& Driver::operator=(Driver&&) = default;
Driver::~Driver() = default;

std::string const Driver::s_tName = utils::tName<Driver>();

void Driver::submit(Scene scene, ScreenRect const& sceneView) {
	m_scene = std::move(scene);
	m_sceneView = sceneView;
	return;
}

void Driver::render(bool bEditor) {
	if (m_uImpl) {
		m_uImpl->render(std::move(m_scene), bEditor);
	}
}

glm::vec2 Driver::screenToN(glm::vec2 const& screenXY) const {
	return m_uImpl ? m_uImpl->screenToN(screenXY) : screenXY;
}

ScreenRect Driver::clampToView(glm::vec2 const& screenXY, glm::vec2 const& nRect, glm::vec2 const& padding) const {
	return m_uImpl ? m_uImpl->clampToView(screenXY, nRect, padding) : ScreenRect::sizeCentre(nRect);
}

void Driver::fill(View& out_view, Viewport const& viewport, Camera const& camera, f32 orthoDepth) const {
	if (m_uImpl) {
		m_uImpl->fill(out_view, viewport, camera, orthoDepth);
	}
}

Driver::Impl::Impl(Info const& info, Driver* pOwner) : m_context(info.contextInfo), m_pDriver(pOwner), m_window(info.windowID) {
	m_name = fmt::format("{}:{}", Driver::s_tName, m_window);
	create(info.frameCount);
}

Driver::Impl::~Impl() {
	destroy();
}

void Driver::Impl::create(u8 frameCount) {
	if (m_renderPass.renderPass == vk::RenderPass()) {
		m_renderPass = rd::createSingleRenderPass(m_context.colourFormat(), m_context.depthFormat());
	}
	if (m_frames.empty() && frameCount > 0) {
		m_frameCount = frameCount;
		// Descriptors
		auto sets = rd::allocateSets(frameCount);
		ENSURE(sets.size() == (std::size_t)frameCount, "Invalid descriptor sets!");
		m_frames.reserve((std::size_t)frameCount);
		for (u8 idx = 0; idx < frameCount; ++idx) {
			FrameSync frame;
			frame.set = sets[(std::size_t)idx];
			frame.renderReady = g_device.device.createSemaphore({});
			frame.presentReady = g_device.device.createSemaphore({});
			frame.drawing = g_device.createFence(true);
			// Commands
			vk::CommandPoolCreateInfo commandPoolCreateInfo;
			commandPoolCreateInfo.queueFamilyIndex = g_device.queues.graphics.familyIndex;
			commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
			frame.commandPool = g_device.device.createCommandPool(commandPoolCreateInfo);
			vk::CommandBufferAllocateInfo allocInfo;
			allocInfo.commandPool = frame.commandPool;
			allocInfo.level = vk::CommandBufferLevel::ePrimary;
			allocInfo.commandBufferCount = 1;
			frame.commandBuffer = g_device.device.allocateCommandBuffers(allocInfo).front();
			m_frames.push_back(std::move(frame));
		}
		logD("[{}] created", m_name);
	}
	return;
}

void Driver::Impl::destroy() {
	if (!m_frames.empty()) {
		g_device.waitIdle();
		for (auto& frame : m_frames) {
			frame.set.destroy();
			g_device.destroy(frame.set.m_bufferPool, frame.set.m_samplerPool, frame.commandPool, frame.framebuffer, frame.drawing, frame.renderReady,
							 frame.presentReady);
		}
		g_device.destroy(m_renderPass.renderPass);
		m_renderPass = {};
		m_frames.clear();
		m_index = 0;
		m_drawnFrames = 0;
		m_texCount = {0, 0};
		logD("[{}] destroyed", m_name);
	}
	return;
}

void Driver::Impl::update() {
}

bool Driver::Impl::render(Driver::Scene scene, bool bExtGUI) {
	bool const bEmpty =
		(scene.batches.empty() || std::all_of(scene.batches.begin(), scene.batches.end(), [](auto const& batch) -> bool { return batch.drawables.empty(); }));

	auto& frame = frameSync();
	g_device.waitFor(frame.drawing);
	if (bExtGUI) {
		ext_gui::render();
	}
	auto const push = bEmpty ? PCDeq() : render::write(frameSync(), scene, m_texCount);
	auto target = m_context.acquireNextImage(frame.renderReady, frame.drawing);
	bool bRendered = false;
	if (target) {
		g_device.destroy(frame.framebuffer);
		frame.framebuffer = g_device.createFramebuffer(m_renderPass.renderPass, target->attachments(), target->extent);
		u64 tris = 0;
		if (push.empty()) {
			static auto const c = colours::black;
			vk::ClearColorValue const colour = std::array{c.r.toF32(), c.g.toF32(), c.b.toF32(), c.a.toF32()};
			vk::ClearDepthStencilValue const depth = {scene.clear.depthStencil.x, (u32)scene.clear.depthStencil.y};
			RenderCmd cmd(frame.commandBuffer, m_renderPass.renderPass, frame.framebuffer, target->extent, {colour, depth});
			if (bExtGUI) {
				ext_gui::renderDrawData(frame.commandBuffer);
			}
		} else {
			PassInfo info{m_context, frameSync(), scene, push, *target, m_renderPass, m_pDriver->m_sceneView, bExtGUI};
			tris = render::pass(info);
		}
		if (render::submit(m_context, frameSync())) {
			next();
			m_pDriver->m_stats.trisDrawn = tris;
			bRendered = true;
		}
	}
	return bRendered;
}

u64 Driver::Impl::framesDrawn() const {
	return m_drawnFrames;
}

u8 Driver::Impl::virtualFrameCount() const {
	return m_frameCount;
}

glm::vec2 Driver::Impl::screenToN(glm::vec2 const& screenXY) const {
	auto pWindow = WindowImpl::windowImpl(m_window);
	if (pWindow) {
		auto const size = pWindow->windowSize();
		return {screenXY.x / (f32)size.x, screenXY.y / (f32)size.y};
	}
	return {};
}

ScreenRect Driver::Impl::clampToView(glm::vec2 const& screenXY, glm::vec2 const& nRect, glm::vec2 const& padding) const {
	auto pWindow = WindowImpl::windowImpl(m_window);
	if (pWindow) {
		auto const size = pWindow->windowSize();
		auto const half = nRect * 0.5f;
		f32 const x = std::clamp(screenXY.x, 0.0f + padding.x, (f32)size.x - padding.x);
		f32 const y = std::clamp(screenXY.y, 0.0f + padding.y, (f32)size.y - padding.y);
		f32 const nX = std::clamp(x / (f32)size.x, 0.0f + half.x, 1.0f - half.x);
		f32 const nY = std::clamp(y / (f32)size.y, 0.0f + half.y, 1.0f - half.y);
		return ScreenRect::sizeCentre(nRect, {nX, nY});
	}
	return ScreenRect::sizeCentre(nRect);
}

bool Driver::Impl::initExtGUI() const {
	ext_gui::Info guiInfo;
	guiInfo.renderPass = m_renderPass.renderPass;
	guiInfo.imageCount = m_frameCount;
	guiInfo.minImageCount = 2;
	guiInfo.glfwWindow = WindowImpl::nativeHandle(m_window);
	return ext_gui::init(guiInfo);
}

ColourSpace Driver::Impl::colourSpace() const {
	if (m_context.colourFormat() == vk::Format::eB8G8R8A8Srgb) {
		return ColourSpace::eSRGBNonLinear;
	}
	return ColourSpace::eRGBLinear;
}

vk::PresentModeKHR Driver::Impl::presentMode() const {
	return m_context.m_metadata.presentMode;
}

std::vector<vk::PresentModeKHR> const& Driver::Impl::presentModes() const {
	return m_context.m_metadata.presentModes;
}

bool Driver::Impl::setPresentMode(vk::PresentModeKHR mode) {
	auto const& modes = presentModes();
	if (auto search = std::find(modes.begin(), modes.end(), mode); search != modes.end()) {
		m_context.m_metadata.info.options.presentModes = {mode};
		return m_context.recreateSwapchain();
	}
	return false;
}

void Driver::Impl::fill(Driver::View& out_view, Viewport const& viewport, Camera const& camera, f32 orthoDepth) {
	auto const ifbsize = m_context.framebufferSize();
	glm::vec2 const fbSize = {ifbsize.x <= 0 ? 1.0f : (f32)ifbsize.x, ifbsize.y <= 0 ? 1.0f : (f32)ifbsize.y};
	auto const fRenderArea = fbSize * viewport.size();
	out_view.pos_v = camera.position;
	out_view.mat_v = camera.view();
	out_view.mat_p = camera.perspective(fRenderArea.x / (fRenderArea.y <= 0.0f ? 1.0f : fRenderArea.y));
	out_view.mat_vp = out_view.mat_p * out_view.mat_v;
	out_view.mat_ui = camera.ui({fbSize, orthoDepth});
}

void Driver::Impl::onFramebufferResize() {
	m_context.onFramebufferResize();
	return;
}

FrameSync& Driver::Impl::frameSync() {
	ENSURE(m_index < m_frames.size(), "Invalid index!");
	return m_frames[m_index];
}

FrameSync const& Driver::Impl::frameSync() const {
	ENSURE(m_index < m_frames.size(), "Invalid index!");
	return m_frames[m_index];
}

void Driver::Impl::next() {
	m_index = (m_index + 1) % m_frames.size();
	++m_drawnFrames;
	return;
}
} // namespace le::gfx::render

namespace le::gfx {
vk::Viewport render::viewport(vk::Extent2D extent, ScreenRect const& nRect, glm::vec2 const& depth) {
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

vk::Rect2D render::scissor(vk::Extent2D extent, ScreenRect const& nRect) {
	vk::Rect2D scissor;
	glm::vec2 const size = nRect.size();
	scissor.offset.x = (s32)(nRect.lt.x * (f32)extent.width);
	scissor.offset.y = (s32)(nRect.lt.y * (f32)extent.height);
	scissor.extent.width = (u32)(size.x * (f32)extent.width);
	scissor.extent.height = (u32)(size.y * (f32)extent.height);
	return scissor;
}

render::PCDeq render::write(FrameSync& out_frame, Driver::Scene& out_scene, TexCounts& out_texCount) {
	auto const mg = colours::magenta.toVec4();
	rd::StorageBuffers ssbos;
	TexSet diffuse, specular;
	PCDeq push;
	auto const white = res::find<res::Texture>("textures/white");
	auto const black = res::find<res::Texture>("textures/black");
	auto const blank = res::find<res::Texture>("cubemaps/blank");
	ENSURE(white && black && blank, "Default textures missing!");
	if (!allReady(*white, *black, *blank)) {
		return {};
	}
	diffuse.add(*white);
	specular.add(*black);
	bool bSkybox = false;
	res::Texture cubemap = *blank;
	if (out_scene.view.skybox.cubemap.status() == res::Status::eReady) {
		out_scene.view.skybox.pipeline.flags.reset(gfx::Pipeline::Flag::eDepthWrite);
		auto mesh = res::find<res::Mesh>("meshes/cube");
		ENSURE(mesh, "Cube mesh not found!");
		out_scene.batches.push_front({{}, {}, {{{*mesh}, Transform::s_identity, out_scene.view.skybox.pipeline}}});
		cubemap = out_scene.view.skybox.cubemap;
		bSkybox = true;
	}
	u32 objectID = 0;
	for (auto& batch : out_scene.batches) {
		push.push_back({});
		for (auto& [meshes, t, _] : batch.drawables) {
			ENSURE(!meshes.empty(), "Mesh is null!");
			Transform const& transform = t;
			for (auto mesh : meshes) {
				auto const& info = res::info(mesh);
				rd::PushConstants pc;
				pc.objectID = objectID;
				ssbos.models.ssbo.push_back(transform.model());
				ssbos.normals.ssbo.push_back(transform.normalModel());
				ssbos.materials.ssbo.push_back({info.material.material.info(), info.material.dropColour});
				ssbos.tints.ssbo.push_back(info.material.tint.toVec4());
				ssbos.flags.ssbo.push_back(0);
				if (bSkybox) {
					bSkybox = false;
					ssbos.flags.ssbo[objectID] |= rd::Flags::eSKYBOX;
				}
				if (info.material.flags.test(res::Material::Flag::eLit)) {
					ssbos.flags.ssbo[objectID] |= rd::Flags::eLIT;
				}
				if (info.material.flags.test(res::Material::Flag::eOpaque)) {
					ssbos.flags.ssbo[objectID] |= rd::Flags::eOPAQUE;
				}
				if (info.material.flags.test(res::Material::Flag::eDropColour)) {
					ssbos.flags.ssbo[objectID] |= rd::Flags::eDROP_COLOUR;
				}
				if (info.material.flags.test(res::Material::Flag::eUI)) {
					ssbos.flags.ssbo[objectID] |= rd::Flags::eUI;
				}
				if (info.material.flags.test(res::Material::Flag::eTextured)) {
					ssbos.flags.ssbo[objectID] |= rd::Flags::eTEXTURED;
					if (info.material.diffuse.status() == res::Status::eReady) {
						pc.diffuseID = diffuse.add(info.material.diffuse);
					} else {
						ssbos.tints.ssbo[objectID] = mg;
						pc.diffuseID = 0;
					}
					if (info.material.specular.status() == res::Status::eReady) {
						pc.specularID = specular.add(info.material.specular);
					}
				}
				push.back().push_back(pc);
				++objectID;
			}
		}
	}
	for (u32 idx = diffuse.total(); idx < out_texCount.diffuse; ++idx) {
		diffuse.textures.push_back(*white);
	}
	for (u32 idx = specular.total(); idx < out_texCount.specular; ++idx) {
		specular.textures.push_back(*black);
	}
	out_texCount = {diffuse.total(), specular.total()};
	rd::View view(out_scene.view, (u32)out_scene.dirLights.size());
	std::copy(out_scene.dirLights.begin(), out_scene.dirLights.end(), std::back_inserter(ssbos.dirLights.ssbo));
	out_frame.set.writeCubemap(cubemap);
	out_frame.set.writeDiffuse(diffuse.textures);
	out_frame.set.writeSpecular(specular.textures);
	out_frame.set.writeSSBOs(ssbos);
	out_frame.set.writeView(view);
	return push;
}

u64 render::pass(PassInfo const& info) {
	RenderContext const& context = info.context;
	PCDeq const& push = info.push;
	Driver::Scene const& scene = info.scene;
	FrameSync const& frame = info.frame;
	RenderTarget const& target = info.target;
	ENSURE(!push.empty(), "No push constants!");
	auto const c = scene.clear.colour;
	vk::ClearColorValue const colour = std::array{c.r.toF32(), c.g.toF32(), c.b.toF32(), c.a.toF32()};
	vk::ClearDepthStencilValue const depth = {scene.clear.depthStencil.x, (u32)scene.clear.depthStencil.y};
	RenderCmd cmd(frame.commandBuffer, info.pass.renderPass, frame.framebuffer, target.extent, {colour, depth});
	std::size_t batchIdx = 0;
	std::size_t drawableIdx = 0;
	u64 tris = 0;
	for (auto& batch : scene.batches) {
		auto const vp = batch.bIgnoreGameView ? batch.viewport : batch.viewport.adjust(info.view);
		auto const sc = batch.bIgnoreGameView ? batch.scissor : batch.scissor.adjust(info.view);
		cmd.setViewportScissor(viewport(context.m_swapchain.extent, vp), scissor(context.m_swapchain.extent, sc));
		for (auto& [meshes, pTransform, pipe] : batch.drawables) {
			for (auto mesh : meshes) {
				auto const& meshInfo = res::info(mesh);
				auto pImpl = res::impl(mesh);
				if (pImpl && mesh.status() == res::Status::eReady && meshInfo.triCount > 0) {
					tris += meshInfo.triCount;
					auto const& impl = pipes::find(pipe, info.pass);
					std::vector const sets = {frame.set.m_bufferSet, frame.set.m_samplerSet};
					cmd.bindResources<rd::PushConstants>(impl, sets, vkFlags::vertFragShader, 0, push[batchIdx][drawableIdx]);
					cmd.bindVertexBuffers(0, pImpl->vbo.buffer.buffer, (vk::DeviceSize)0);
					if (pImpl->ibo.count > 0) {
						cmd.bindIndexBuffer(pImpl->ibo.buffer.buffer, 0, vk::IndexType::eUint32);
						cmd.drawIndexed(pImpl->ibo.count, 1, 0, 0, 0);
					} else {
						cmd.draw(pImpl->vbo.count, 1, 0, 0);
					}
				}
				++drawableIdx;
			}
		}
		drawableIdx = 0;
		++batchIdx;
	}
	if (info.bExtGUI) {
		ext_gui::renderDrawData(frame.commandBuffer);
	}
	return tris;
}

bool render::submit(RenderContext& out_context, FrameSync const& frame) {
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags const waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame.renderReady;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame.commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame.presentReady;
	g_device.device.resetFences(frame.drawing);
	g_device.queues.graphics.queue.submit(submitInfo, frame.drawing);
	if (out_context.present(frame.presentReady)) {
		return true;
	}
	return false;
}
} // namespace le::gfx
