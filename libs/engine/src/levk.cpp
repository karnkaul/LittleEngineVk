#include <thread>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stb/stb_image.h>
#include "core/io.hpp"
#include "core/jobs.hpp"
#include "core/log.hpp"
#include "core/maths.hpp"
#include "core/map_store.hpp"
#include "core/transform.hpp"
#include "core/services.hpp"
#include "engine/levk.hpp"
#include "engine/window/window.hpp"
#include "gfx/info.hpp"
#include "gfx/presenter.hpp"
#include "gfx/renderer.hpp"
#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "gfx/draw/resource_descriptors.hpp"
#include "gfx/draw/pipeline.hpp"
#include "gfx/draw/resources.hpp"
#include "gfx/draw/shader.hpp"
#include "gfx/draw/texture.hpp"
#include "gfx/draw/vertex.hpp"
#include "window/window_impl.hpp"

namespace le
{
namespace
{
std::unique_ptr<IOReader> g_uReader;
}

s32 engine::run(s32 argc, char** argv)
{
	Services services;
	try
	{
		services.add<os::Service, os::Args>({argc, argv});
		services.add<log::Service, std::string_view>("debug.log");
		services.add<jobs::Service, u8>(4);
		services.add<Window::Service>();

		NativeWindow dummyWindow({});
		gfx::InitInfo initInfo;
#if 1 || defined(LEVK_DEBUG) // Always enabled, for time being
		initInfo.options.flags.set(gfx::InitInfo::Flag::eValidation);
#endif
		initInfo.config.instanceExtensions = WindowImpl::vulkanInstanceExtensions();
		initInfo.config.createTempSurface = [&](vk::Instance instance) { return WindowImpl::createSurface(instance, dummyWindow); };

		services.add<gfx::Service>(std::move(initInfo));
		services.add<gfx::Resources::Service>();
	}
	catch (std::exception const& e)
	{
		LOG_E("Failed to initialise services: {}", e.what());
		return 1;
	}

	auto const exeDirectory = os::dirPath(os::Dir::eExecutable);
	auto [dataPath, bResult] = FileReader::findUpwards(exeDirectory, {"data"});
	LOGIF_I(true, "Executable located at: {}", exeDirectory.generic_string());
	if (bResult)
	{
		LOG_D("Found data at: {}", dataPath.generic_string());
	}
	else
	{
		LOG_E("Could not locate data!");
		return 1;
	}
	g_uReader = std::make_unique<FileReader>(dataPath);
	for (s32 i = 0; i < 10; ++i)
	{
		jobs::enqueue([i]() {
			std::this_thread::sleep_for(stdch::milliseconds(maths::randomRange(10, 1000)));
			log::fmtLog(log::Level::eDebug, "Job #{}", __FILE__, __LINE__, i);
		});
	}
	try
	{
		auto const vertShaderPath = dataPath / "shaders/tutorial.vert";
		auto const fragShaderPath = dataPath / "shaders/tutorial.frag";

		gfx::Shader::Info tutorialShaderInfo;
		std::array shaderIDs = {stdfs::path("shaders/tutorial.vert"), stdfs::path("shaders/tutorial.frag")};
		ASSERT(g_uReader->checkPresences(ArrayView<stdfs::path const>(shaderIDs)), "Shaders missing!");
		tutorialShaderInfo.pReader = g_uReader.get();
		tutorialShaderInfo.codeIDMap.at((size_t)gfx::ShaderType::eVertex) = shaderIDs.at(0);
		tutorialShaderInfo.codeIDMap.at((size_t)gfx::ShaderType::eFragment) = shaderIDs.at(1);
		auto pTutorialShader = gfx::g_pResources->create<gfx::Shader>("shaders/tutorial", tutorialShaderInfo);
		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		auto const& qfi = gfx::g_info.queueFamilyIndices;
		commandPoolCreateInfo.queueFamilyIndex = qfi.transfer;
		auto transferPool = gfx::g_info.device.createCommandPool(commandPoolCreateInfo);

		gfx::Vertex const triangle0Verts[] = {
			gfx::Vertex{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			gfx::Vertex{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			gfx::Vertex{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		};

		gfx::Vertex const quad0Verts[] = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
										  {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
										  {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
										  {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}};
		u32 const quad0Indices[] = {0, 1, 2, 2, 3, 0};

		auto createDeviceBuffer = [](vk::DeviceSize size, vk::BufferUsageFlags flags) -> gfx::Buffer {
			gfx::BufferInfo info;
			info.size = size;
			info.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
			info.usage = flags | vk::BufferUsageFlagBits::eTransferDst;
			info.queueFlags = gfx::QFlag::eGraphics | gfx::QFlag::eTransfer;
			info.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
			return gfx::vram::createBuffer(info);
		};

		gfx::Buffer triangle0VB, quad0VB, quad0IB;
		{
			std::vector<vk::Fence> transferFences;
			triangle0VB = createDeviceBuffer(sizeof(triangle0Verts), vk::BufferUsageFlagBits::eVertexBuffer);
			quad0VB = createDeviceBuffer(sizeof(quad0Verts), vk::BufferUsageFlagBits::eVertexBuffer);
			quad0IB = createDeviceBuffer(sizeof(quad0Indices), vk::BufferUsageFlagBits::eIndexBuffer);
			transferFences.push_back(gfx::vram::stage(triangle0VB, triangle0Verts));
			transferFences.push_back(gfx::vram::stage(quad0VB, quad0Verts));
			transferFences.push_back(gfx::vram::stage(quad0IB, quad0Indices));
			gfx::waitAll(transferFences);
		}

		gfx::ImageInfo imageInfo;
		imageInfo.queueFlags = gfx::QFlag::eTransfer | gfx::QFlag::eGraphics;
		imageInfo.createInfo.format = vk::Format::eR8G8B8A8Srgb;
		imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.createInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
		imageInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		imageInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.createInfo.imageType = vk::ImageType::e2D;
		imageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.createInfo.mipLevels = 1;
		imageInfo.createInfo.arrayLayers = 1;
		gfx::Texture::Info textureInfo;
		textureInfo.pReader = g_uReader.get();
		textureInfo.imgID.channels = 4;
		textureInfo.imgID.id = dataPath / "textures/texture.jpg";
		auto pTexture = gfx::g_pResources->create<gfx::Texture>("textures/texture.jpg", textureInfo);
		auto pBlank = gfx::g_pResources->get<gfx::Texture>("textures/blank");

		Window w0, w1;
		Window::Info info0;
		info0.config.size = {1280, 720};
		info0.config.title = "LittleEngineVk Demo";
		auto info1 = info0;
		// info1.config.mode = Window::Mode::eBorderlessFullscreen;
		info1.config.title += " 2";
		info1.config.centreOffset = {100, 100};
		info1.options.colourSpaces = {ColourSpace::eRGBLinear};
		bool bRecreate0 = false, bRecreate1 = false;
		bool bClose0 = false, bClose1 = false;
		bool bWF0 = false;
		auto registerInput = [](Window& self, Window& other, bool& bRecreate, bool& bClose, std::shared_ptr<int>& token) {
			token = self.registerInput([&](Key key, Action action, Mods mods) {
				if (self.isOpen() && key == Key::eW && action == Action::eRelease && mods & Mods::eCONTROL)
				{
					bClose = true;
				}
				if (!other.isOpen() && (key == Key::eT || key == Key::eN) && action == Action::eRelease && mods & Mods::eCONTROL)
				{
					bRecreate = true;
				}
			});
		};
		std::shared_ptr<s32> token0, token1, wf0Token;
		wf0Token = w0.registerInput([&bWF0](Key key, Action action, Mods mods) {
			if (key == Key::eP && action == Action::eRelease && mods & Mods::eCONTROL)
			{
				bWF0 = !bWF0;
			}
		});
		registerInput(w0, w1, bRecreate1, bClose0, token0);
		registerInput(w1, w0, bRecreate0, bClose1, token1);
		auto createRenderer = [pTutorialShader](gfx::Pipeline* pPipeline, gfx::Presenter** ppPresenter, WindowID id) {
			*ppPresenter = WindowImpl::presenter(id);
			gfx::Pipeline::Info pipelineInfo;
			pipelineInfo.pShader = pTutorialShader;
			pipelineInfo.setLayouts = {gfx::rd::g_setLayouts.layouts.at((size_t)gfx::rd::Type::eUniformBuffer)};
			pipelineInfo.name = "default";
			pPipeline->create(std::move(pipelineInfo));
			gfx::Renderer::Info info;
			info.frameCount = 2;
			info.pPresenter = *ppPresenter;
			return std::make_unique<gfx::Renderer>(info);
		};
		if (w0.create(info0) && w1.create(info1))
		{
			std::unique_ptr<gfx::Renderer> uRenderer0, uRenderer1;
			gfx::Presenter *pPresenter0 = nullptr, *pPresenter1 = nullptr;
			gfx::Pipeline pipeline0(w0.id()), pipeline0wf(w0.id()), pipeline1(w1.id());

			if (w0.isOpen())
			{
				gfx::Pipeline::Info pipelineInfo;
				pipelineInfo.name = "wireframe";
				pipelineInfo.setLayouts = {gfx::rd::g_setLayouts.layouts.at((size_t)gfx::rd::Type::eUniformBuffer)};
				pipelineInfo.pShader = pTutorialShader;
				pipelineInfo.polygonMode = vk::PolygonMode::eLine;
				pipelineInfo.staticLineWidth = gfx::g_info.lineWidth(3.0f);
				pipeline0wf.create(pipelineInfo);
				uRenderer0 = createRenderer(&pipeline0, &pPresenter0, w0.id());
			}
			if (w1.isOpen())
			{
				uRenderer1 = createRenderer(&pipeline1, &pPresenter1, w1.id());
			}

			gfx::ubo::View view0;
			gfx::ubo::View view1;
			Transform transform0;

			Time t = Time::elapsed();
			while (w0.isOpen() || w1.isOpen())
			{
				[[maybe_unused]] Time dt = Time::elapsed() - t;
				t = Time::elapsed();

				{
					gfx::g_pResources->update();
					pipeline0.update();
					pipeline0wf.update();
					pipeline1.update();

					// Update matrices
					transform0.setOrientation(
						glm::rotate(transform0.orientation(), glm::radians(dt.to_s() * 10), glm::vec3(0.0f, 1.0f, 0.0f)));
					view0.mat_v = view1.mat_v =
						glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					if (w0.isOpen())
					{
						auto const size = w0.framebufferSize();
						if (size.x > 0 && size.y > 0)
						{
							auto proj = glm::perspective(glm::radians(45.0f), (f32)size.x / size.y, 0.1f, 10.0f);
							proj[1][1] *= -1;
							view0.mat_vp = proj * view0.mat_v;
						}
					}
					if (w1.isOpen())
					{
						auto const size = w1.framebufferSize();
						if (size.x > 0 && size.y > 0)
						{
							auto proj = glm::perspective(glm::radians(45.0f), (f32)size.x / size.y, 0.1f, 10.0f);
							proj[1][1] *= -1;
							view1.mat_vp = proj * view1.mat_v;
						}
					}
				}

				if (w0.isClosing())
				{
					w0.destroy();
				}
				if (w1.isClosing())
				{
					w1.destroy();
				}
				if (bRecreate0)
				{
					bRecreate0 = false;
					w0.create(info0);
					gfx::Pipeline::Info pipelineInfo;
					pipelineInfo.name = "wireframe";
					pipelineInfo.setLayouts = {gfx::rd::g_setLayouts.layouts.at((size_t)gfx::rd::Type::eUniformBuffer)};
					pipelineInfo.pShader = pTutorialShader;
					pipelineInfo.polygonMode = vk::PolygonMode::eLine;
					pipelineInfo.staticLineWidth = gfx::g_info.lineWidth(3.0f);
					pipeline0wf.create(pipelineInfo);
					uRenderer0 = createRenderer(&pipeline0, &pPresenter0, w0.id());
				}
				if (bRecreate1)
				{
					bRecreate1 = false;
					w1.create(info1);
					uRenderer1 = createRenderer(&pipeline1, &pPresenter1, w1.id());
				}
				if (bClose0)
				{
					bClose0 = false;
					w0.close();
				}
				if (bClose1)
				{
					bClose1 = false;
					w1.close();
				}
				Window::pollEvents();
				// Render
				try
				{
					auto drawFrame = [pBlank, &transform0](gfx::ubo::View* pView, gfx::Renderer* pRenderer, vk::Pipeline pipeline,
												   vk::PipelineLayout layout, vk::Buffer vertexBuffer, vk::Buffer indexBuffer,
												   u32 vertCount, u32 indexCount, gfx::Texture* pTexture) -> bool {
						gfx::ClearValues clear;
						clear.colour = Colour(0x030203ff);
						gfx::ubo::Flags flags;
						flags.isTextured = indexCount > 0 ? 1 : 0;
						auto write = [&](gfx::rd::Set& set) {
							set.writeView(*pView);
							set.writeFlags(flags);
							if (pTexture)
							{
								set.writeDiffuse(*pTexture);
							}
							else
							{
								set.writeDiffuse(*pBlank);
							}
						};
						auto draw = [&](gfx::Renderer::FrameDriver const& driver) {
							vk::Viewport viewport = pRenderer->transformViewport();
							vk::Rect2D scissor = pRenderer->transformScissor();
							driver.commandBuffer.setViewport(0, viewport);
							driver.commandBuffer.setScissor(0, scissor);
							driver.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
							vk::DeviceSize offsets[] = {0};
							driver.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, driver.set.m_set, {});
							driver.commandBuffer.pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4),
															   glm::value_ptr(transform0.model()));
							driver.commandBuffer.bindVertexBuffers(gfx::Vertex::binding, 1, &vertexBuffer, offsets);
							if (indexCount > 0)
							{
								driver.commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
								driver.commandBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
							}
							else
							{
								driver.commandBuffer.draw(vertCount, 1, 0, 0);
							}
						};
						return pRenderer->render(write, draw, clear);
					};

					if (w0.isOpen() && uRenderer0.get())
					{
						drawFrame(&view0, uRenderer0.get(), bWF0 ? pipeline0wf.pipeline() : pipeline0.pipeline(), pipeline0.layout(),
								  quad0VB.buffer, quad0IB.buffer, (u32)arraySize(quad0Verts), (u32)arraySize(quad0Indices), pTexture);
					}
					if (w1.isOpen() && uRenderer1.get())
					{
						drawFrame(&view1, uRenderer1.get(), pipeline1.pipeline(), pipeline1.layout(), triangle0VB.buffer, {},
								  (u32)arraySize(triangle0Verts), 0, nullptr);
						// drawFrame(&view1, pPresenter1, pipeline1, layout1, quad0VBIB.buffer, (u32)arraySize(quad0Verts),
						//	  (u32)arraySize(quad0Indices));
					}
				}
				catch (std::exception const& e)
				{
					LOG_E("EXCEPTION!\n\t{}", e.what());
				}
			}
			gfx::g_info.device.waitIdle();
			gfx::vkDestroy(transferPool);
			gfx::vram::release(triangle0VB, quad0VB, quad0IB);
		}
	}
	catch (std::exception const& e)
	{
		LOG_E("Exception!\n\t{}", e.what());
	}
	std::this_thread::sleep_for(stdch::milliseconds(maths::randomRange(10, 1000)));
	return 0;
}
} // namespace le
