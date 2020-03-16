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
#include "vuk/utils.hpp"
#include "vuk/shader.hpp"
#include "vuk/draw/vertex.hpp"
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

		vuk::Vertex triangle0Verts[] = {
			vuk::Vertex{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			vuk::Vertex{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			vuk::Vertex{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		};

		auto createVertexBuffer = [&](u32 size, vk::CommandPool pool, vk::Queue queue) -> vuk::Resource<vk::Buffer> {
			vuk::BufferData bufferData;
			bufferData.size = size;
			bufferData.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			bufferData.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc;
			auto [stagingBuf, stagingMem] = vuk::createBuffer(bufferData);
			auto pData = vuk::g_info.device.mapMemory(stagingMem, 0, size);
			std::memcpy(pData, triangle0Verts, size);
			vuk::g_info.device.unmapMemory(stagingMem);
			bufferData.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
			bufferData.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;
			auto result = vuk::createBuffer(bufferData);
			vuk::TransferOp transfer;
			transfer.queue = queue;
			transfer.pool = pool;
			vuk::copyBuffer(stagingBuf, result.resource, size, &transfer);
			vuk::g_info.device.waitForFences(transfer.transferred, true, maxVal<u64>());
			vuk::vkDestroy(transfer.transferred);
			vuk::g_info.device.freeCommandBuffers(transfer.pool, transfer.commandBuffer);
			vuk::g_info.device.destroyBuffer(stagingBuf);
			vuk::g_info.device.freeMemory(stagingMem);
			return result;
		};

		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		auto const& qfi = vuk::g_info.queueFamilyIndices;
		commandPoolCreateInfo.queueFamilyIndex = qfi.transfer;
		auto transferPool = vuk::g_info.device.createCommandPool(commandPoolCreateInfo);
		auto [triangle0VB, triangle0VBmemory] = createVertexBuffer(sizeof(triangle0Verts), transferPool, vuk::g_info.queues.transfer);

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
		auto createRenderer = [&](vk::Pipeline* pPipeline, vuk::Context** ppContext, WindowID id) {
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
					auto drawFrame = [&](vuk::Context* pContext, vk::Pipeline pipeline, std::vector<vk::Buffer> vertexBuffers) -> bool {
						auto [bResult, commandBuffer] = pContext->beginRenderPass();
						if (bResult)
						{
							vk::Viewport viewport = pContext->transformViewport();
							vk::Rect2D scissor = pContext->transformScissor();
							commandBuffer.setViewport(0, viewport);
							commandBuffer.setScissor(0, scissor);

							commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
							vk::DeviceSize offsets[] = {0};
							commandBuffer.bindVertexBuffers(0, (u32)vertexBuffers.size(), vertexBuffers.data(), offsets);
							commandBuffer.draw((u32)arraySize(triangle0Verts), 1, 0, 0);

							commandBuffer.endRenderPass();
							commandBuffer.end();

							return pContext->submitPresent();
						}
						return false;
					};

					if (w0.isOpen())
					{
						drawFrame(pContext0, pipeline0, {triangle0VB});
					}
					if (w1.isOpen())
					{
						drawFrame(pContext1, pipeline1, {triangle0VB});
					}
				}
				catch (std::exception const& e)
				{
					LOG_E("EXCEPTION!\n\t{}", e.what());
				}
			}
			vuk::g_info.device.waitIdle();
			vuk::vkDestroy(pipeline0, pipeline1);
			vuk::vkDestroy(transferPool);
			vuk::g_info.device.destroyBuffer(triangle0VB);
			vuk::g_info.device.freeMemory(triangle0VBmemory);
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
