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
#include "engine/assets/resources.hpp"
#include "engine/gfx/mesh.hpp"
#include "engine/gfx/shader.hpp"
#include "engine/gfx/texture.hpp"
#include "engine/window/window.hpp"
#include "gfx/info.hpp"
#include "gfx/presenter.hpp"
#include "gfx/renderer.hpp"
#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "gfx/draw/resource_descriptors.hpp"
#include "gfx/draw/pipeline.hpp"
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
#if defined(LEVK_DEBUG)
		initInfo.options.flags.set(gfx::InitInfo::Flag::eValidation);
#endif
		if (os::isDefined("validation"))
		{
			LOG_I("Validation layers requested, enabling...");
			initInfo.options.flags.set(gfx::InitInfo::Flag::eValidation);
		}
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
		gfx::Shader::Info tutorialShaderInfo;
		std::array shaderIDs = {stdfs::path("shaders/tutorial.vert"), stdfs::path("shaders/tutorial.frag")};
		ASSERT(g_uReader->checkPresences(ArrayView<stdfs::path const>(shaderIDs)), "Shaders missing!");
		tutorialShaderInfo.pReader = g_uReader.get();
		tutorialShaderInfo.codeIDMap.at((size_t)gfx::Shader::Type::eVertex) = shaderIDs.at(0);
		tutorialShaderInfo.codeIDMap.at((size_t)gfx::Shader::Type::eFragment) = shaderIDs.at(1);
		auto pTutorialShader = gfx::g_pResources->create<gfx::Shader>("shaders/tutorial", tutorialShaderInfo);
		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		auto const& qfi = gfx::g_info.queueFamilyIndices;
		commandPoolCreateInfo.queueFamilyIndex = qfi.transfer;
		auto transferPool = gfx::g_info.device.createCommandPool(commandPoolCreateInfo);

		gfx::Mesh::Info triangle0info;
		triangle0info.geometry.vertices = {
			gfx::Vertex{{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 0.0f}},
			gfx::Vertex{{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
			gfx::Vertex{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		};
		gfx::Mesh* pTriangle0 = gfx::g_pResources->create<gfx::Mesh>("meshes/triangle0", triangle0info);

		// clang-format off
		auto const dz = 0.25f;
		gfx::Mesh::Info quad0info;
		quad0info.geometry.vertices = {
			{{-0.5f, -0.5f, -dz}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{0.5f, -0.5f, -dz}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			{{0.5f, 0.5f, -dz}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
			{{-0.5f, 0.5f, -dz}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
			{{-0.5f, -0.5f, dz}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{0.5f, -0.5f, dz}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			{{0.5f, 0.5f, dz}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
			{{-0.5f, 0.5f, dz}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
		};
		quad0info.geometry.indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};
		// clang-format on
		gfx::Mesh* pQuad0 = gfx::g_pResources->create<gfx::Mesh>("meshes/quad0", quad0info);

		gfx::Material::Info texturedInfo;
		texturedInfo.flags.set(gfx::Material::Flag::eTextured);
		auto pTextured = gfx::g_pResources->create<gfx::Material>("materials/textured", texturedInfo);

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
		textureInfo.imgID.assetID = "textures/texture.jpg";
		pQuad0->m_material.pMaterial = pTextured;
		pQuad0->m_material.pDiffuse = gfx::g_pResources->create<gfx::Texture>(textureInfo.imgID.assetID, textureInfo);

		Window w0, w1;
		auto pW0 = WindowImpl::windowImpl(w0.id());
		auto pW1 = WindowImpl::windowImpl(w1.id());
		Window::Info info0;
		info0.config.size = {1280, 720};
		info0.config.title = "LittleEngineVk Demo";
		info0.config.virtualFrameCount = 2;
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
		auto createPipeline = [pTutorialShader](gfx::Renderer* pRenderer, std::string_view name,
												vk::PolygonMode mode = vk::PolygonMode::eFill, f32 lineWidth = 3.0f) -> gfx::Pipeline* {
			gfx::Pipeline::Info pipelineInfo;
			pipelineInfo.pShader = pTutorialShader;
			pipelineInfo.setLayouts = {gfx::rd::g_setLayout};
			pipelineInfo.name = name;
			pipelineInfo.polygonMode = mode;
			pipelineInfo.staticLineWidth = lineWidth;
			vk::PushConstantRange pcRange;
			pcRange.size = sizeof(gfx::PushConstants);
			pcRange.stageFlags = gfx::vkFlags::vertFragShader;
			pipelineInfo.pushConstantRanges = {pcRange};
			return pRenderer->createPipeline(std::move(pipelineInfo));
		};
		if (w0.create(info0) && w1.create(info1))
		{
			gfx::Pipeline *pPipeline0 = nullptr, *pPipeline0wf = nullptr, *pPipeline1 = nullptr;

			if (w0.isOpen())
			{
				auto pRenderer0 = WindowImpl::windowImpl(w0.id())->m_uRenderer.get();
				pPipeline0 = createPipeline(pRenderer0, "default");
				pPipeline0wf = createPipeline(pRenderer0, "wireframe", vk::PolygonMode::eLine);
			}
			if (w1.isOpen())
			{
				auto pRenderer1 = WindowImpl::windowImpl(w1.id())->m_uRenderer.get();
				pPipeline1 = createPipeline(pRenderer1, "default");
			}

			gfx::rd::View view0;
			gfx::rd::View view1;
			Transform transform0;

			Time t = Time::elapsed();
			while (w0.isOpen() || w1.isOpen())
			{
				[[maybe_unused]] Time dt = Time::elapsed() - t;
				t = Time::elapsed();

				static Time fpsLogTime = Time::from_s(3.0f);
				static Time fpsLogElapsed;
				static Time fpsElapsed;
				static u32 fps = 0;
				static u32 frames = 0;
				++frames;
				fpsElapsed += dt;
				fpsLogElapsed += dt;
				if (fpsElapsed >= Time::from_s(1.0f))
				{
					fps = frames;
					frames = 0;
					fpsElapsed = Time();
				}
				if (fpsLogElapsed >= fpsLogTime)
				{
					LOG_I("dt: {}, FPS: {}", dt.to_s() * 1000, fps);
					fps = 0;
					fpsLogElapsed = Time();
				}

				{
					gfx::g_pResources->update();
					if (pW0->m_uRenderer)
					{
						pW0->m_uRenderer->update();
					}
					if (pW1->m_uRenderer)
					{
						pW1->m_uRenderer->update();
					}

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
					pPipeline0 = createPipeline(pW0->m_uRenderer.get(), "default");
					pPipeline0wf = createPipeline(pW0->m_uRenderer.get(), "wireframe", vk::PolygonMode::eLine);
				}
				if (bRecreate1)
				{
					bRecreate1 = false;
					w1.create(info1);
					pPipeline1 = createPipeline(pW1->m_uRenderer.get(), "default");
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
					if (w0.isOpen())
					{
						gfx::Renderer::Scene scene;
						scene.clear.colour = Colour(0x030203ff);
						scene.pView = &view0;
						if (pQuad0->isReady() && pTriangle0->isReady())
						{
							auto pPipeline = bWF0 ? pPipeline0wf : pPipeline0;
							gfx::Renderer::Batch batch;
							batch.drawables = {{pQuad0, &transform0, pPipeline}, {pTriangle0, &transform0, pPipeline}};
							scene.batches.push_back(std::move(batch));
							pW0->m_uRenderer->render(scene);
						}
					}
					if (w1.isOpen() && pTriangle0->isReady())
					{
						gfx::Renderer::Scene scene;
						scene.clear.colour = Colour(0x030203ff);
						scene.pView = &view1;
						if (pTriangle0->isReady())
						{
							gfx::Renderer::Batch batch;
							batch.drawables = {{pTriangle0, &transform0, pPipeline1}};
							scene.batches.push_back(std::move(batch));
							pW1->m_uRenderer->render(scene);
						}
					}
				}
				catch (std::exception const& e)
				{
					LOG_E("EXCEPTION!\n\t{}", e.what());
				}
			}
			gfx::g_info.device.waitIdle();
			gfx::vkDestroy(transferPool);
		}
	}
	catch (std::exception const& e)
	{
		LOG_E("Exception!\n\t{}", e.what());
	}
	return 0;
}
} // namespace le
