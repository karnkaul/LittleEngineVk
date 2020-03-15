#include <glm/gtx/quaternion.hpp>
#include "core/io.hpp"
#include "core/jobs.hpp"
#include "core/log.hpp"
#include "core/maths.hpp"
#include "core/map_store.hpp"
#include "core/services.hpp"
#include "engine/levk.hpp"
#include "engine/window/window.hpp"
#include "vuk/info.hpp"
#include "vuk/context.hpp"
#include "vuk/rendering.hpp"
#include "vuk/shader.hpp"
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
		vuk::InitData initData;
#if 1 || defined(LEVK_DEBUG) // Always enabled, for time being
		initData.options.flags.set(vuk::InitData::Flag::eValidation);
#endif
		initData.config.instanceExtensions = WindowImpl::vulkanInstanceExtensions();
		initData.config.createTempSurface = [&](vk::Instance instance) { return WindowImpl::createSurface(instance, dummyWindow); };

		services.add<vuk::Service>(std::move(initData));
	}
	catch (std::exception const& e)
	{
		LOG_E("Failed to initialise services: {}", e.what());
		return 1;
	}

	auto const exeDirectory = os::dirPath(os::Dir::eExecutable);
	auto [result, dataPath] = FileReader::findUpwards(exeDirectory, {"data"});
	LOGIF_I(true, "Executable located at: {}", exeDirectory.generic_string());
	if (result == IOReader::Code::eSuccess)
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
		vuk::Shader::Data tutorialData;
		std::array shaderIDs = {stdfs::path("shaders/tutorial.vert.spv"), stdfs::path("shaders/tutorial.frag.spv")};
		ASSERT(g_uReader->checkPresences(ArrayView<stdfs::path const>(shaderIDs)), "Shaders missing!");
		tutorialData.pReader = g_uReader.get();
		tutorialData.codeIDMap[vuk::Shader::Type::eVertex] = shaderIDs.at(0);
		tutorialData.codeIDMap[vuk::Shader::Type::eFragment] = shaderIDs.at(1);
		vuk::Shader tutorialShader(std::move(tutorialData));

		Window w0, w1;
		Window::Data data0;
		data0.config.size = {1280, 720};
		data0.config.title = "LittleEngineVk Demo";
		auto data1 = data0;
		data1.config.title += " 2";
		data1.config.centreOffset = {100, 100};
		data1.options.colourSpace = ColourSpace::eRGBLinear;
		bool bRecreate0 = false, bRecreate1 = false;
		auto registerInput = [](Window& listen, Window& recreate, bool& bRecreate, std::shared_ptr<int>& token) {
			token = listen.registerInput([&](Key key, Action action, Mods mods) {
				if (!recreate.isOpen() && key == Key::eW && action == le::Action::eRelease && mods & Mods::eCONTROL)
				{
					bRecreate = true;
				}
			});
		};
		std::shared_ptr<s32> token0, token1;
		registerInput(w0, w1, bRecreate1, token0);
		registerInput(w1, w0, bRecreate0, token1);
		auto createRenderer = [&](vk::Pipeline* pPipeline, vuk::Context** ppContext, WindowID id)
		{
			vuk::vkDestroy(*pPipeline);
			*ppContext = WindowImpl::context(id);
			vk::PipelineLayout pipelineLayout = vuk::g_info.device.createPipelineLayout(vk::PipelineLayoutCreateInfo());
			vuk::PipelineData pipelineData;
			pipelineData.pShader = &tutorialShader;
			pipelineData.renderPass = (*ppContext)->m_renderPass;
			*pPipeline = vuk::createPipeline(pipelineLayout, pipelineData);
			vuk::vkDestroy(pipelineLayout);
			return;
		};

		if (w0.create(data0) && w1.create(data1))
		{
			vuk::Context* pContext0 = nullptr;
			vuk::Context* pContext1 = nullptr;
			vk::Pipeline pipeline0;
			vk::Pipeline pipeline1;

			createRenderer(&pipeline0, &pContext0, w0.id());
			createRenderer(&pipeline1, &pContext1, w1.id());

			Time t = Time::elapsed();
			while (w0.isOpen() || w1.isOpen())
			{
				[[maybe_unused]] Time dt = Time::elapsed() - t;
				t = Time::elapsed();

				std::this_thread::sleep_for(stdch::milliseconds(10));
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
					w0.create(data0);
					createRenderer(&pipeline0, &pContext0, w0.id());
				}
				if (bRecreate1)
				{
					bRecreate1 = false;
					w1.create(data1);
					createRenderer(&pipeline1, &pContext1, w1.id());
				}
				Window::pollEvents();
				// Render
				try
				{
					auto drawFrame = [&](vuk::Context* pContext, vk::Pipeline pipeline) {
						auto commandBuffer = pContext->beginRenderPass();
						vk::Viewport viewport = pContext->transformViewport();
						vk::Rect2D scissor = pContext->transformScissor();
						commandBuffer.setViewport(0, viewport);
						commandBuffer.setScissor(0, scissor);

						commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
						commandBuffer.draw(3, 1, 0, 0);

						commandBuffer.endRenderPass();
						commandBuffer.end();

						pContext->submitPresent();
					};

					if (w0.isOpen())
					{
						drawFrame(pContext0, pipeline0);
					}
					if (w1.isOpen())
					{
						drawFrame(pContext1, pipeline1);
					}
				}
				catch (std::exception const& e)
				{
					LOG_E("EXCEPTION!\n\t{}", e.what());
				}
			}
			vuk::g_info.device.waitIdle();
			vuk::vkDestroy(pipeline0, pipeline1);
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
