#include <algorithm>
#include <unordered_set>
#include <glm/gtc/matrix_transform.hpp>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/transform.hpp"
#include "core/utils.hpp"
#include "engine/assets/resources.hpp"
#include "engine/editor/editor.hpp"
#include "engine/gfx/mesh.hpp"
#include "gui.hpp"
#include "info.hpp"
#include "utils.hpp"
#include "pipeline_impl.hpp"
#include "renderer_impl.hpp"
#include "resource_descriptors.hpp"
#include "window/window_impl.hpp"

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

ScreenRect::ScreenRect(glm::vec4 const& ltrb) noexcept : left(ltrb.x), top(ltrb.y), right(ltrb.z), bottom(ltrb.w) {}

ScreenRect ScreenRect::sizeTL(glm::vec2 const& size, glm::vec2 const& leftTop)
{
	return ScreenRect({leftTop.x, leftTop.y, leftTop.x + size.x, leftTop.y + size.y});
}

ScreenRect ScreenRect::sizeCentre(glm::vec2 const& size, glm::vec2 const& centre)
{
	auto const leftTop = centre - (glm::vec2(0.5f) * size);
	return ScreenRect({leftTop.x, leftTop.y, leftTop.x + size.x, leftTop.y + size.y});
}

glm::vec2 ScreenRect::size() const
{
	return glm::vec2(right - left, bottom - top);
}

f32 ScreenRect::aspect() const
{
	glm::vec2 const s = size();
	return s.x / s.y;
}

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

void Renderer::update()
{
	if (m_uImpl)
	{
		m_uImpl->update();
	}
}

void Renderer::render(Scene scene)
{
	if (m_uImpl)
	{
		m_uImpl->render(std::move(scene));
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

RendererImpl::RendererImpl(Info const& info, Renderer* pOwner)
	: m_presenter(info.presenterInfo), m_pRenderer(pOwner), m_window(info.windowID)
{
	m_name = fmt::format("{}:{}", Renderer::s_tName, m_window);
	create(info.frameCount);
	Pipeline::Info pipelineInfo;
	pipelineInfo.name = "default";
	m_pipes.pDefault = createPipeline(std::move(pipelineInfo));
	pipelineInfo.name = "skybox";
	pipelineInfo.bDepthWrite = false;
	m_pipes.pSkybox = createPipeline(std::move(pipelineInfo));
	m_bGUI = info.bGUI;
	if (m_bGUI)
	{
		gui::Info guiInfo;
		guiInfo.renderPass = m_presenter.m_renderPass;
		guiInfo.imageCount = m_frameCount;
		guiInfo.minImageCount = 2;
		guiInfo.window = m_window;
		if (!gui::init(guiInfo))
		{
			LOG_E("[{}] Failed to initialise GUI!", m_name);
			m_bGUI = false;
		}
#if defined(LEVK_EDITOR)
		else
		{
			editor::init();
		}
#endif
	}
}

RendererImpl::~RendererImpl()
{
	if (m_bGUI)
	{
		deferred::release([]() {
#if defined(LEVK_EDITOR)
			editor::deinit();
#endif
			gui::deinit();
		});
	}
	m_pipelines.clear();
	destroy();
}

void RendererImpl::create(u8 frameCount)
{
	if (m_frames.empty() && frameCount > 0)
	{
		m_frameCount = frameCount;
		// Descriptors
		auto descriptorSetup = rd::allocateSets(frameCount);
		ASSERT(descriptorSetup.set.size() == (size_t)frameCount, "Invalid setup!");
		m_descriptorPool = descriptorSetup.descriptorPool;
		for (u8 idx = 0; idx < frameCount; ++idx)
		{
			RendererImpl::FrameSync frame;
			frame.set = descriptorSetup.set.at((size_t)idx);
			frame.renderReady = g_info.device.createSemaphore({});
			frame.presentReady = g_info.device.createSemaphore({});
			frame.drawing = createFence(true);
			// Commands
			vk::CommandPoolCreateInfo commandPoolCreateInfo;
			commandPoolCreateInfo.queueFamilyIndex = g_info.queues.graphics.familyIndex;
			commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
			frame.commandPool = g_info.device.createCommandPool(commandPoolCreateInfo);
			vk::CommandBufferAllocateInfo allocInfo;
			allocInfo.commandPool = frame.commandPool;
			allocInfo.level = vk::CommandBufferLevel::ePrimary;
			allocInfo.commandBufferCount = 1;
			frame.commandBuffer = g_info.device.allocateCommandBuffers(allocInfo).front();
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
		g_info.device.waitIdle();
		for (auto& frame : m_frames)
		{
			frame.set.destroy();
			vkDestroy(frame.commandPool, frame.framebuffer, frame.drawing, frame.renderReady, frame.presentReady);
		}
		vkDestroy(m_descriptorPool);
		m_descriptorPool = vk::DescriptorPool();
		m_frames.clear();
		m_index = 0;
		m_drawnFrames = 0;
		LOG_D("[{}] destroyed", m_name);
	}
	return;
}

Pipeline* RendererImpl::createPipeline(Pipeline::Info info)
{
	PipelineImpl::Info implInfo;
	implInfo.renderPass = m_presenter.m_renderPass;
	implInfo.pShader = info.pShader;
	implInfo.setLayouts = {rd::g_setLayout};
	implInfo.name = info.name;
	implInfo.polygonMode = g_polygonModeMap.at((size_t)info.polygonMode);
	implInfo.cullMode = g_cullModeMap.at((size_t)info.cullMode);
	implInfo.staticLineWidth = info.lineWidth;
	implInfo.bBlend = info.bBlend;
	implInfo.bDepthTest = info.bDepthTest;
	implInfo.bDepthWrite = info.bDepthWrite;
	implInfo.window = m_window;
	vk::PushConstantRange pcRange;
	pcRange.size = sizeof(rd::PushConstants);
	pcRange.stageFlags = vkFlags::vertFragShader;
	implInfo.pushConstantRanges = {pcRange};
	m_pipelines.push_back({});
	auto& pipeline = m_pipelines.back();
	pipeline.m_uImpl = std::make_unique<PipelineImpl>(&pipeline);
	if (!pipeline.m_uImpl->create(std::move(implInfo)))
	{
		m_pipelines.pop_back();
		return nullptr;
	}
	return &pipeline;
}

void RendererImpl::update()
{
	switch (m_presenter.m_state)
	{
	case Presenter::State::eDestroyed:
	{
		destroy();
		return;
	}
	case Presenter::State::eSwapchainDestroyed:
	{
		destroy();
		return;
	}
	case Presenter::State::eSwapchainRecreated:
	{
		destroy();
		create(m_frameCount);
		break;
	}
	default:
	{
		break;
	}
	}
	if (m_bGUI)
	{
		gfx::gui::newFrame();
	}
	for (auto& pipeline : m_pipelines)
	{
		pipeline.m_uImpl->update();
	}
	return;
}

bool RendererImpl::render(Renderer::Scene scene)
{
	if (scene.batches.empty()
		|| std::all_of(scene.batches.begin(), scene.batches.end(), [](auto const& batch) -> bool { return batch.drawables.empty(); }))
	{
		return false;
	}
	if (m_bGUI)
	{
		gui::render();
	}
	auto const mg = colours::magenta;
	auto& frame = frameSync();
	waitFor(frame.drawing);
	// Write sets
	u32 objectID = 0;
	rd::SSBOs ssbos;
	TexSet diffuse;
	TexSet specular;
	std::deque<std::deque<rd::PushConstants>> push;
	auto const pWhite = Resources::inst().get<Texture>("textures/white");
	auto const pBlack = Resources::inst().get<Texture>("textures/black");
	diffuse.add(pWhite);
	specular.add(pBlack);
	frame.set.writeCubemap(*Resources::inst().get<Cubemap>("cubemaps/blank"));
	bool bSkybox = false;
	if (scene.view.skybox.pCubemap)
	{
		if (!scene.view.skybox.pPipeline)
		{
			scene.view.skybox.pPipeline = m_pipes.pSkybox;
		}
		ASSERT(scene.view.skybox.pPipeline, "Pipeline is null!");
		auto pTransform = &Transform::s_identity;
		auto pMesh = Resources::inst().get<Mesh>("meshes/cube");
		scene.batches.push_front({scene.view.skybox.viewport, {}, {{{pMesh}, pTransform, scene.view.skybox.pPipeline}}});
		frame.set.writeCubemap(*scene.view.skybox.pCubemap);
		bSkybox = true;
	}
	u64 tris = 0;
	for (auto& batch : scene.batches)
	{
		push.push_back({});
		for (auto& [meshes, pTransform, _] : batch.drawables)
		{
			ASSERT(!meshes.empty() && pTransform, "Mesh / Transform is null!");
			auto mat_m = pTransform->model();
			auto mat_n = pTransform->normalModel();
			for (auto pMesh : meshes)
			{
				ASSERT(pMesh, "Mesh is null!");
				tris += pMesh->m_triCount;
				rd::PushConstants pc;
				pc.objectID = objectID;
				ssbos.models.ssbo.push_back(mat_m);
				ssbos.normals.ssbo.push_back(mat_n);
				ssbos.materials.ssbo.push_back({*pMesh->m_material.pMaterial, pMesh->m_material.dropColour});
				ssbos.tints.ssbo.push_back(pMesh->m_material.tint.toVec4());
				ssbos.flags.ssbo.push_back(0);
				if (bSkybox)
				{
					bSkybox = false;
					ssbos.flags.ssbo.at(objectID) |= rd::SSBOFlags::eSKYBOX;
				}
				if (pMesh->m_material.flags.isSet(Material::Flag::eLit))
				{
					ssbos.flags.ssbo.at(objectID) |= rd::SSBOFlags::eLIT;
				}
				if (pMesh->m_material.flags.isSet(Material::Flag::eOpaque))
				{
					ssbos.flags.ssbo.at(objectID) |= rd::SSBOFlags::eOPAQUE;
				}
				if (pMesh->m_material.flags.isSet(Material::Flag::eDropColour))
				{
					ssbos.flags.ssbo.at(objectID) |= rd::SSBOFlags::eDROP_COLOUR;
				}
				if (pMesh->m_material.flags.isSet(Material::Flag::eUI))
				{
					ssbos.flags.ssbo.at(objectID) |= rd::SSBOFlags::eUI;
				}
				if (pMesh->m_material.flags.isSet(Material::Flag::eTextured))
				{
					ssbos.flags.ssbo.at(objectID) |= rd::SSBOFlags::eTEXTURED;
					if (!pMesh->m_material.pDiffuse)
					{
						ssbos.tints.ssbo.at(objectID) = mg.toVec4();
						pc.diffuseID = 0;
					}
					else
					{
						pc.diffuseID = diffuse.add(pMesh->m_material.pDiffuse);
					}
					if (pMesh->m_material.pSpecular)
					{
						pc.specularID = specular.add(pMesh->m_material.pSpecular);
					}
				}
				push.back().push_back(pc);
				++objectID;
			}
		}
	}
	u32 lastDiffuseID = diffuse.total();
	u32 lastSpecularID = specular.total();
	m_maxDiffuseID = std::max(m_maxDiffuseID, lastDiffuseID);
	m_maxSpecularID = std::max(m_maxSpecularID, lastSpecularID);
	for (u32 id = lastDiffuseID; id < m_maxDiffuseID; ++id)
	{
		diffuse.add(pWhite);
	}
	for (u32 id = lastSpecularID; id < m_maxSpecularID; ++id)
	{
		specular.add(pBlack);
	}
	rd::UBOView view(scene.view, (u32)scene.dirLights.size());
	std::copy(scene.dirLights.begin(), scene.dirLights.end(), std::back_inserter(ssbos.dirLights.ssbo));
	frame.set.writeDiffuse(diffuse.textures);
	frame.set.writeSpecular(specular.textures);
	frame.set.writeSSBOs(ssbos);
	frame.set.writeView(view);
	// Acquire
	auto [acquire, bResult] = m_presenter.acquireNextImage(frame.renderReady, frame.drawing);
	if (!bResult)
	{
		return false;
	}
	// Framebuffer
	vkDestroy(frame.framebuffer);
	vk::FramebufferCreateInfo createInfo;
	createInfo.attachmentCount = (u32)acquire.attachments.size();
	createInfo.pAttachments = acquire.attachments.data();
	createInfo.renderPass = acquire.renderPass;
	createInfo.width = acquire.swapchainExtent.width;
	createInfo.height = acquire.swapchainExtent.height;
	createInfo.layers = 1;
	frame.framebuffer = g_info.device.createFramebuffer(createInfo);
	// RenderPass
	auto const c = scene.clear.colour;
	std::array const clearColour = {c.r.toF32(), c.g.toF32(), c.b.toF32(), c.a.toF32()};
	vk::ClearColorValue const colour = clearColour;
	vk::ClearDepthStencilValue const depth = {scene.clear.depthStencil.x, (u32)scene.clear.depthStencil.y};
	std::array<vk::ClearValue, 2> const clearValues = {colour, depth};
	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = acquire.renderPass;
	renderPassInfo.framebuffer = frame.framebuffer;
	renderPassInfo.renderArea.extent = acquire.swapchainExtent;
	renderPassInfo.clearValueCount = (u32)clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	// Begin
	auto const& commandBuffer = frame.commandBuffer;
	vk::CommandBufferBeginInfo beginInfo;
	commandBuffer.begin(beginInfo);
	commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	// Draw
	std::unordered_set<PipelineImpl*> pipelines;
	size_t batchIdx = 0;
	size_t drawableIdx = 0;
	for (auto& batch : scene.batches)
	{
		commandBuffer.setViewport(0, transformViewport(batch.viewport));
		commandBuffer.setScissor(0, transformScissor(batch.scissor));
		vk::Pipeline pipeline;
		for (auto& [meshes, pTransform, pPipeline] : batch.drawables)
		{
			for (auto pMesh : meshes)
			{
				if (pMesh->isReady() && pMesh->m_triCount > 0)
				{
					if (!pPipeline)
					{
						pPipeline = m_pipes.pDefault;
					}
					ASSERT(pPipeline, "Pipeline is null!");
					if (pipeline != pPipeline->m_uImpl->m_pipeline)
					{
						pipeline = pPipeline->m_uImpl->m_pipeline;
						commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
					}
					auto layout = pPipeline->m_uImpl->m_layout;
					commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, frame.set.m_descriptorSet, {});
					commandBuffer.pushConstants<rd::PushConstants>(layout, vkFlags::vertFragShader, 0, push.at(batchIdx).at(drawableIdx));
					commandBuffer.bindVertexBuffers(0, pMesh->m_uImpl->vbo.buffer.buffer, (vk::DeviceSize)0);
					if (pMesh->m_uImpl->ibo.count > 0)
					{
						commandBuffer.bindIndexBuffer(pMesh->m_uImpl->ibo.buffer.buffer, 0, vk::IndexType::eUint32);
						commandBuffer.drawIndexed(pMesh->m_uImpl->ibo.count, 1, 0, 0, 0);
					}
					else
					{
						commandBuffer.draw(pMesh->m_uImpl->vbo.count, 1, 0, 0);
					}
					pipelines.insert(pPipeline->m_uImpl.get());
				}
				++drawableIdx;
			}
		}
		drawableIdx = 0;
		++batchIdx;
	}
	if (m_bGUI)
	{
		gui::renderDrawData(commandBuffer);
	}
	commandBuffer.endRenderPass();
	commandBuffer.end();
	// Submit
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags const waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame.renderReady;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame.presentReady;
	g_info.device.resetFences(frame.drawing);
	g_info.queues.graphics.queue.submit(submitInfo, frame.drawing);
	if (m_presenter.present(frame.presentReady))
	{
		next();
		m_pRenderer->m_stats.trisDrawn = tris;
		return true;
	}
	return false;
}

vk::Viewport RendererImpl::transformViewport(ScreenRect const& nRect, glm::vec2 const& depth) const
{
	vk::Viewport viewport;
	auto const& extent = m_presenter.m_swapchain.extent;
	glm::vec2 const size = {nRect.right - nRect.left, nRect.bottom - nRect.top};
	viewport.minDepth = depth.x;
	viewport.maxDepth = depth.y;
	viewport.width = size.x * (f32)extent.width;
	viewport.height = -(size.y * (f32)extent.height);
	viewport.x = nRect.left * (f32)extent.width;
	viewport.y = nRect.top * (f32)extent.height - (f32)viewport.height;
	return viewport;
}

vk::Rect2D RendererImpl::transformScissor(ScreenRect const& nRect) const
{
	vk::Rect2D scissor;
	glm::vec2 const size = {nRect.right - nRect.left, nRect.bottom - nRect.top};
	scissor.offset.x = (s32)(nRect.left * (f32)m_presenter.m_swapchain.extent.width);
	scissor.offset.y = (s32)(nRect.top * (f32)m_presenter.m_swapchain.extent.height);
	scissor.extent.width = (u32)(size.x * (f32)m_presenter.m_swapchain.extent.width);
	scissor.extent.height = (u32)(size.y * (f32)m_presenter.m_swapchain.extent.height);
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

void RendererImpl::onFramebufferResize()
{
	m_presenter.onFramebufferResize();
	return;
}

RendererImpl::FrameSync& RendererImpl::frameSync()
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
} // namespace le::gfx
