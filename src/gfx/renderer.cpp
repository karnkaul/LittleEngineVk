#include <algorithm>
#include <unordered_set>
#include <glm/gtc/matrix_transform.hpp>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/transform.hpp>
#include <core/utils.hpp>
#include <engine/assets/resources.hpp>
#include <engine/gfx/mesh.hpp>
#include <gfx/device.hpp>
#include <gfx/ext_gui.hpp>
#include <gfx/pipeline_impl.hpp>
#include <gfx/render_cmd.hpp>
#include <gfx/renderer_impl.hpp>
#include <gfx/resource_descriptors.hpp>
#include <editor/editor.hpp>
#include <window/window_impl.hpp>

namespace le::gfx
{
namespace
{
struct TexSet final
{
	std::unordered_map<Texture const*, u32> idxMap;
	std::deque<Texture const*> textures;

	u32 add(Texture const* pTex);
	u32 total() const;
};

u32 TexSet::add(Texture const* pTex)
{
	if (auto search = idxMap.find(pTex); search != idxMap.end())
	{
		return search->second;
	}
	u32 const idx = (u32)textures.size();
	idxMap[pTex] = idx;
	textures.push_back(pTex);
	return idx;
}

u32 TexSet::total() const
{
	return (u32)textures.size();
}
} // namespace

Renderer::Renderer() = default;
Renderer::Renderer(Renderer&&) = default;
Renderer& Renderer::operator=(Renderer&&) = default;
Renderer::~Renderer() = default;

std::string const Renderer::s_tName = utils::tName<Renderer>();

Pipeline* Renderer::createPipeline(Pipeline::Info info)
{
	if (m_uImpl)
	{
		return m_uImpl->createPipeline(std::move(info));
	}
	return nullptr;
}

void Renderer::submit(Scene scene, ScreenRect const& sceneView)
{
	m_scene = std::move(scene);
	m_sceneView = sceneView;
	return;
}

void Renderer::render(bool bEditor)
{
	if (m_uImpl)
	{
		m_uImpl->render(std::move(m_scene), bEditor);
	}
}

glm::vec2 Renderer::screenToN(glm::vec2 const& screenXY) const
{
	return m_uImpl ? m_uImpl->screenToN(screenXY) : screenXY;
}

ScreenRect Renderer::clampToView(glm::vec2 const& screenXY, glm::vec2 const& nViewport, glm::vec2 const& padding) const
{
	return m_uImpl ? m_uImpl->clampToView(screenXY, nViewport, padding) : ScreenRect::sizeCentre(nViewport);
}

RendererImpl::RendererImpl(Info const& info, Renderer* pOwner) : m_context(info.contextInfo), m_pRenderer(pOwner), m_window(info.windowID)
{
	m_name = fmt::format("{}:{}", Renderer::s_tName, m_window);
	create(info.frameCount);
	Pipeline::Info pipelineInfo;
	pipelineInfo.name = "default";
	m_pipes.pDefault = createPipeline(std::move(pipelineInfo));
	pipelineInfo.name = "skybox";
	pipelineInfo.flags.reset(Pipeline::Flag::eDepthWrite);
	m_pipes.pSkybox = createPipeline(std::move(pipelineInfo));
}

RendererImpl::~RendererImpl()
{
	m_pipelines.clear();
	destroy();
}

void RendererImpl::create(u8 frameCount)
{
	if (m_renderPass == vk::RenderPass())
	{
		m_renderPass = rd::createSingleRenderPass(m_context.colourFormat(), m_context.depthFormat());
	}
	if (m_frames.empty() && frameCount > 0)
	{
		m_frameCount = frameCount;
		// Descriptors
		auto setLayouts = rd::allocateSets(frameCount);
		ASSERT(setLayouts.sets.size() == (size_t)frameCount, "Invalid descriptor sets!");
		m_samplerLayout = setLayouts.samplerLayout;
		m_frames.reserve((size_t)frameCount);
		for (u8 idx = 0; idx < frameCount; ++idx)
		{
			RendererImpl::FrameSync frame;
			frame.set = setLayouts.sets.at((size_t)idx);
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
		LOG_D("[{}] created", m_name);
	}
	return;
}

void RendererImpl::destroy()
{
	if (!m_frames.empty())
	{
		g_device.device.waitIdle();
		for (auto& frame : m_frames)
		{
			frame.set.destroy();
			g_device.destroy(frame.set.m_bufferPool, frame.set.m_samplerPool, frame.commandPool, frame.framebuffer, frame.drawing,
							 frame.renderReady, frame.presentReady);
		}
		g_device.destroy(m_samplerLayout, m_renderPass);
		m_samplerLayout = vk::DescriptorSetLayout();
		m_renderPass = vk::RenderPass();
		m_frames.clear();
		m_index = 0;
		m_drawnFrames = 0;
		m_texCount = {0, 0};
		LOG_D("[{}] destroyed", m_name);
	}
	return;
}

void RendererImpl::update()
{
#if defined(LEVK_ASSET_HOT_RELOAD)
	for (auto& pipeline : m_pipelines)
	{
		pipeline.m_uImpl->pollShaders();
	}
#endif
}

Pipeline* RendererImpl::createPipeline(Pipeline::Info info)
{
	PipelineImpl::Info implInfo;
	implInfo.name = info.name;
	implInfo.vertexBindings = rd::vbo::vertexBindings();
	implInfo.vertexAttributes = rd::vbo::vertexAttributes();
	implInfo.pushConstantRanges = rd::PushConstants::ranges();
	implInfo.renderPass = m_renderPass;
	implInfo.polygonMode = g_polygonModeMap.at((size_t)info.polygonMode);
	implInfo.cullMode = g_cullModeMap.at((size_t)info.cullMode);
	implInfo.frontFace = g_frontFaceMap.at((size_t)info.frontFace);
	implInfo.samplerLayout = m_samplerLayout;
	implInfo.window = m_window;
	implInfo.staticLineWidth = info.lineWidth;
	implInfo.pShader = info.pShader;
	implInfo.flags = info.flags;
	Pipeline pipeline;
	pipeline.m_uImpl = std::make_unique<PipelineImpl>();
	if (!pipeline.m_uImpl->create(std::move(implInfo)))
	{
		return nullptr;
	}
	m_pipelines.push_back(std::move(pipeline));
	return &m_pipelines.back();
}

bool RendererImpl::render(Renderer::Scene scene, bool bExtGUI)
{
	bool const bEmpty = (scene.batches.empty() || std::all_of(scene.batches.begin(), scene.batches.end(), [](auto const& batch) -> bool {
							 return batch.drawables.empty();
						 }));
	if (bExtGUI)
	{
		ext_gui::render();
	}
	auto& frame = frameSync();
	g_device.waitFor(frame.drawing);
	auto const push = bEmpty ? PCDeq() : writeSets(scene);
	auto [target, result] = m_context.acquireNextImage(frame.renderReady, frame.drawing);
	bool bRecreate = false;
	bool bRendered = false;
	switch (result)
	{
	case RenderContext::Outcome::eSuccess:
	{
		g_device.destroy(frame.framebuffer);
		frame.framebuffer = g_device.createFramebuffer(m_renderPass, target.attachments(), target.extent);
		u64 tris = 0;
		if (bEmpty)
		{
			static auto const c = colours::black;
			vk::ClearColorValue const colour = std::array{c.r.toF32(), c.g.toF32(), c.b.toF32(), c.a.toF32()};
			vk::ClearDepthStencilValue const depth = {scene.clear.depthStencil.x, (u32)scene.clear.depthStencil.y};
			RenderCmd cmd(frame.commandBuffer, m_renderPass, frame.framebuffer, target.extent, {colour, depth});
		}
		else
		{
			tris = doRenderPass(scene, push, target, bExtGUI);
		}
		result = submit();
		bRecreate = result == RenderContext::Outcome::eSwapchainRecreated;
		if (result == RenderContext::Outcome::eSuccess)
		{
			next();
			m_pRenderer->m_stats.trisDrawn = tris;
			bRendered = true;
		}
		break;
	}
	case RenderContext::Outcome::eSwapchainRecreated:
	{
		bRecreate = true;
		break;
	}
	default:
		break;
	}
	if (bRecreate)
	{
		destroy();
		create(m_frameCount);
	}
	return bRendered;
}

vk::Viewport RendererImpl::transformViewport(ScreenRect const& nRect, glm::vec2 const& depth) const
{
	vk::Viewport viewport;
	auto const& extent = m_context.m_swapchain.extent;
	glm::vec2 const size = {nRect.right - nRect.left, nRect.bottom - nRect.top};
	viewport.minDepth = depth.x;
	viewport.maxDepth = depth.y;
	viewport.width = size.x * (f32)extent.width;
	viewport.height = -(size.y * (f32)extent.height); // flip viewport about X axis
	viewport.x = nRect.left * (f32)extent.width;
	viewport.y = nRect.top * (f32)extent.height - (f32)viewport.height;
	return viewport;
}

vk::Rect2D RendererImpl::transformScissor(ScreenRect const& nRect) const
{
	vk::Rect2D scissor;
	auto const& extent = m_context.m_swapchain.extent;
	glm::vec2 const size = {nRect.right - nRect.left, nRect.bottom - nRect.top};
	scissor.offset.x = (s32)(nRect.left * (f32)extent.width);
	scissor.offset.y = (s32)(nRect.top * (f32)extent.height);
	scissor.extent.width = (u32)(size.x * (f32)extent.width);
	scissor.extent.height = (u32)(size.y * (f32)extent.height);
	return scissor;
}

u64 RendererImpl::framesDrawn() const
{
	return m_drawnFrames;
}

u8 RendererImpl::virtualFrameCount() const
{
	return m_frameCount;
}

glm::vec2 RendererImpl::screenToN(glm::vec2 const& screenXY) const
{
	auto pWindow = WindowImpl::windowImpl(m_window);
	if (pWindow)
	{
		auto const size = pWindow->windowSize();
		return {screenXY.x / (f32)size.x, screenXY.y / (f32)size.y};
	}
	return {};
}

ScreenRect RendererImpl::clampToView(glm::vec2 const& screenXY, glm::vec2 const& nViewport, glm::vec2 const& padding) const
{
	auto pWindow = WindowImpl::windowImpl(m_window);
	if (pWindow)
	{
		auto const size = pWindow->windowSize();
		auto const half = nViewport * 0.5f;
		f32 const x = std::clamp(screenXY.x, 0.0f + padding.x, (f32)size.x - padding.x);
		f32 const y = std::clamp(screenXY.y, 0.0f + padding.y, (f32)size.y - padding.y);
		f32 const nX = std::clamp(x / (f32)size.x, 0.0f + half.x, 1.0f - half.x);
		f32 const nY = std::clamp(y / (f32)size.y, 0.0f + half.y, 1.0f - half.y);
		return ScreenRect::sizeCentre(nViewport, {nX, nY});
	}
	return ScreenRect::sizeCentre(nViewport);
}

bool RendererImpl::initExtGUI() const
{
	ext_gui::Info guiInfo;
	guiInfo.renderPass = m_renderPass;
	guiInfo.imageCount = m_frameCount;
	guiInfo.minImageCount = 2;
	guiInfo.window = m_window;
	return ext_gui::init(guiInfo);
}

ColourSpace RendererImpl::colourSpace() const
{
	if (m_context.colourFormat() == vk::Format::eB8G8R8A8Srgb)
	{
		return ColourSpace::eSRGBNonLinear;
	}
	return ColourSpace::eRGBLinear;
}

void RendererImpl::onFramebufferResize()
{
	m_context.onFramebufferResize();
	return;
}

RendererImpl::FrameSync& RendererImpl::frameSync()
{
	ASSERT(m_index < m_frames.size(), "Invalid index!");
	return m_frames.at(m_index);
}

RendererImpl::FrameSync const& RendererImpl::frameSync() const
{
	ASSERT(m_index < m_frames.size(), "Invalid index!");
	return m_frames.at(m_index);
}

void RendererImpl::next()
{
	m_index = (m_index + 1) % m_frames.size();
	++m_drawnFrames;
	return;
}

RendererImpl::PCDeq RendererImpl::writeSets(Renderer::Scene& out_scene)
{
	auto& frame = frameSync();
	auto const mg = colours::magenta.toVec4();
	rd::StorageBuffers ssbos;
	TexSet diffuse, specular;
	PCDeq push;
	auto const pWhite = Resources::inst().get<Texture>("textures/white");
	auto const pBlack = Resources::inst().get<Texture>("textures/black");
	diffuse.add(pWhite);
	specular.add(pBlack);
	bool bSkybox = false;
	auto const* pCubemap = Resources::inst().get<Texture>("cubemaps/blank");
	if (out_scene.view.skybox.pCubemap)
	{
		if (!out_scene.view.skybox.pPipeline)
		{
			out_scene.view.skybox.pPipeline = m_pipes.pSkybox;
		}
		ASSERT(out_scene.view.skybox.pPipeline, "Pipeline is null!");
		auto pTransform = &Transform::s_identity;
		auto pMesh = Resources::inst().get<Mesh>("meshes/cube");
		out_scene.batches.push_front({out_scene.view.skybox.viewport, {}, {{{pMesh}, pTransform, out_scene.view.skybox.pPipeline}}});
		if (out_scene.view.skybox.pCubemap)
		{
			ASSERT(out_scene.view.skybox.pCubemap->isReady(), "Skybox Cubemap is not ready!");
			pCubemap = out_scene.view.skybox.pCubemap;
		}
		bSkybox = true;
	}
	u32 objectID = 0;
	for (auto& batch : out_scene.batches)
	{
		push.push_back({});
		for (auto& [meshes, pTransform, _] : batch.drawables)
		{
			ASSERT(!meshes.empty() && pTransform, "Mesh / Transform is null!");
			for (auto pMesh : meshes)
			{
				ASSERT(pMesh, "Mesh is null!");
				rd::PushConstants pc;
				pc.objectID = objectID;
				ssbos.models.ssbo.push_back(pTransform->model());
				ssbos.normals.ssbo.push_back(pTransform->normalModel());
				ssbos.materials.ssbo.push_back({*pMesh->m_material.pMaterial, pMesh->m_material.dropColour});
				ssbos.tints.ssbo.push_back(pMesh->m_material.tint.toVec4());
				ssbos.flags.ssbo.push_back(0);
				if (bSkybox)
				{
					bSkybox = false;
					ssbos.flags.ssbo.at(objectID) |= rd::Flags::eSKYBOX;
				}
				if (pMesh->m_material.flags.isSet(Material::Flag::eLit))
				{
					ssbos.flags.ssbo.at(objectID) |= rd::Flags::eLIT;
				}
				if (pMesh->m_material.flags.isSet(Material::Flag::eOpaque))
				{
					ssbos.flags.ssbo.at(objectID) |= rd::Flags::eOPAQUE;
				}
				if (pMesh->m_material.flags.isSet(Material::Flag::eDropColour))
				{
					ssbos.flags.ssbo.at(objectID) |= rd::Flags::eDROP_COLOUR;
				}
				if (pMesh->m_material.flags.isSet(Material::Flag::eUI))
				{
					ssbos.flags.ssbo.at(objectID) |= rd::Flags::eUI;
				}
				if (pMesh->m_material.flags.isSet(Material::Flag::eTextured))
				{
					ssbos.flags.ssbo.at(objectID) |= rd::Flags::eTEXTURED;
					if (pMesh->m_material.pDiffuse)
					{
						ASSERT(!pMesh->m_material.pDiffuse->isBusy(), "Texture busy!");
						pc.diffuseID = diffuse.add(pMesh->m_material.pDiffuse);
					}
					else
					{
						ssbos.tints.ssbo.at(objectID) = mg;
						pc.diffuseID = 0;
					}
					if (pMesh->m_material.pSpecular)
					{
						ASSERT(!pMesh->m_material.pSpecular->isBusy(), "Texture busy!");
						pc.specularID = specular.add(pMesh->m_material.pSpecular);
					}
				}
				push.back().push_back(pc);
				++objectID;
			}
		}
	}
	for (u32 idx = diffuse.total(); idx < m_texCount.diffuse; ++idx)
	{
		diffuse.textures.push_back(pWhite);
	}
	for (u32 idx = specular.total(); idx < m_texCount.specular; ++idx)
	{
		specular.textures.push_back(pBlack);
	}
	m_texCount = {diffuse.total(), specular.total()};
	rd::View view(out_scene.view, (u32)out_scene.dirLights.size());
	std::copy(out_scene.dirLights.begin(), out_scene.dirLights.end(), std::back_inserter(ssbos.dirLights.ssbo));
	frame.set.writeCubemap(*pCubemap);
	frame.set.writeDiffuse(diffuse.textures);
	frame.set.writeSpecular(specular.textures);
	frame.set.writeSSBOs(ssbos);
	frame.set.writeView(view);
	return push;
}

u64 RendererImpl::doRenderPass(Renderer::Scene const& scene, PCDeq const& push, RenderTarget const& target, bool bExtGUI) const
{
	auto const& frame = frameSync();
	auto const c = scene.clear.colour;
	vk::ClearColorValue const colour = std::array{c.r.toF32(), c.g.toF32(), c.b.toF32(), c.a.toF32()};
	vk::ClearDepthStencilValue const depth = {scene.clear.depthStencil.x, (u32)scene.clear.depthStencil.y};
	RenderCmd cmd(frame.commandBuffer, m_renderPass, frame.framebuffer, target.extent, {colour, depth});
	std::unordered_set<PipelineImpl*> pipelines;
	size_t batchIdx = 0;
	size_t drawableIdx = 0;
	u64 tris = 0;
	for (auto& batch : scene.batches)
	{
		cmd.setViewportScissor(transformViewport(batch.viewport.adjust(m_pRenderer->m_sceneView)), transformScissor(batch.scissor));
		for (auto& [meshes, pTransform, pPipe] : batch.drawables)
		{
			for (auto pMesh : meshes)
			{
				if (pMesh->isReady() && pMesh->m_triCount > 0)
				{
					tris += pMesh->m_triCount;
					auto pPipeline = pPipe ? pPipe : m_pipes.pDefault;
					ASSERT(pPipeline, "Pipeline is null!");
					[[maybe_unused]] bool bOk = pPipeline->m_uImpl->update(m_renderPass, m_samplerLayout);
					ASSERT(bOk, "Pipeline update failure!");
					std::vector const sets = {frame.set.m_bufferSet, frame.set.m_samplerSet};
					cmd.bindResources<rd::PushConstants>(*pPipeline->m_uImpl, sets, vkFlags::vertFragShader, 0,
														 push.at(batchIdx).at(drawableIdx));
					cmd.bindVertexBuffers(0, pMesh->m_uImpl->vbo.buffer.buffer, (vk::DeviceSize)0);
					if (pMesh->m_uImpl->ibo.count > 0)
					{
						cmd.bindIndexBuffer(pMesh->m_uImpl->ibo.buffer.buffer, 0, vk::IndexType::eUint32);
						cmd.drawIndexed(pMesh->m_uImpl->ibo.count, 1, 0, 0, 0);
					}
					else
					{
						cmd.draw(pMesh->m_uImpl->vbo.count, 1, 0, 0);
					}
					pipelines.insert(pPipeline->m_uImpl.get());
				}
				++drawableIdx;
			}
		}
		drawableIdx = 0;
		++batchIdx;
	}
	if (bExtGUI)
	{
		ext_gui::renderDrawData(frame.commandBuffer);
	}
	return tris;
}

RenderContext::Outcome RendererImpl::submit()
{
	auto& frame = frameSync();
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
	return m_context.present(frame.presentReady);
}
} // namespace le::gfx
