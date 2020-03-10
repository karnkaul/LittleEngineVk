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
#include "vuk/context/swapchain.hpp"
#include "vuk/rendering/pipeline.hpp"
#include "vuk/rendering/shader.hpp"
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
		tutorialData.pReader = g_uReader.get();
		tutorialData.codeIDMap[vuk::Shader::Type::eVertex] = "shaders/tutorial.vert.spv";
		tutorialData.codeIDMap[vuk::Shader::Type::eFragment] = "shaders/tutorial.frag.spv";
		vuk::Shader tutorialShader(std::move(tutorialData));

		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.queueFamilyIndex = vuk::g_info.queueFamilyIndices.graphics;
		vk::CommandPool commandPool = vuk::g_info.device.createCommandPool(commandPoolCreateInfo);

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

		constexpr size_t maxFrames = 2;
		size_t frameIdx = 0;
		struct Swap
		{
			vk::Semaphore render;
			vk::Semaphore present;
			vk::Fence inFlight;
		};
		std::array<Swap, maxFrames> swaps0, swaps1;
		for (auto& swap : swaps0)
		{
			swap.render = vuk::g_info.device.createSemaphore({});
			swap.present = vuk::g_info.device.createSemaphore({});
			vk::FenceCreateInfo createInfo;
			createInfo.flags = vk::FenceCreateFlagBits::eSignaled;
			swap.inFlight = vuk::g_info.device.createFence(createInfo);
		}

		for (auto& swap : swaps1)
		{
			swap.render = vuk::g_info.device.createSemaphore({});
			swap.present = vuk::g_info.device.createSemaphore({});
			vk::FenceCreateInfo createInfo;
			createInfo.flags = vk::FenceCreateFlagBits::eSignaled;
			swap.inFlight = vuk::g_info.device.createFence(createInfo);
		}

		if (w0.create(data0) && w1.create(data1))
		{
			vk::PipelineLayout pipelineLayout = vuk::g_info.device.createPipelineLayout(vk::PipelineLayoutCreateInfo());

			vuk::PipelineData pipelineData;
			pipelineData.pShader = &tutorialShader;
			pipelineData.renderPass = WindowImpl::swapchain(w0.id())->m_defaultRenderPass;
			vk::Pipeline pipeline0 = vuk::createPipeline(pipelineLayout, pipelineData);
			pipelineData.renderPass = WindowImpl::swapchain(w1.id())->m_defaultRenderPass;
			vk::Pipeline pipeline1 = vuk::createPipeline(pipelineLayout, pipelineData);

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
				if (bRecreate1)
				{
					bRecreate1 = false;
					w1.create(data1);
				}
				if (bRecreate0)
				{
					bRecreate0 = false;
					w0.create(data0);
				}

				// Render
				Window::pollEvents();

				try
				{
					auto drawFrame = [&](vuk::Swapchain* pSwapchain, vk::Pipeline pipeline, Swap const& swap) {
						vk::CommandBufferAllocateInfo allocInfo;
						allocInfo.commandPool = commandPool;
						allocInfo.level = vk::CommandBufferLevel::ePrimary;
						allocInfo.commandBufferCount = 1;
						auto commandBuffer = vuk::g_info.device.allocateCommandBuffers(allocInfo).front();
						vk::CommandBufferBeginInfo beginInfo;
						commandBuffer.begin(beginInfo);

						vuk::g_info.device.waitForFences(swap.inFlight, true, maxVal<u64>());
						auto renderPassInfo = pSwapchain->acquireNextImage(swap.render, swap.inFlight);
						std::array<f32, 4> const clearColour = {0.0f, 0.0f, 0.0f, 1.0f};
						vk::ClearValue clearValue(vk::ClearColorValue{clearColour});
						renderPassInfo.clearValueCount = 1;
						renderPassInfo.pClearValues = &clearValue;
						commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

						vk::Viewport viewport;
						viewport.minDepth = 0.0f;
						viewport.maxDepth = 1.0f;
						viewport.height = (f32)renderPassInfo.renderArea.extent.height;
						viewport.width = (f32)renderPassInfo.renderArea.extent.width;
						viewport.x = 0.0f;
						viewport.y = 0.0f;
						commandBuffer.setViewport(0, viewport);

						vk::Rect2D scissor({0, 0}, renderPassInfo.renderArea.extent);
						commandBuffer.setScissor(0, scissor);

						commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, static_cast<vk::Pipeline>(pipeline));
						commandBuffer.draw(3, 1, 0, 0);

						commandBuffer.endRenderPass();
						commandBuffer.end();

						vk::SubmitInfo submitInfo;
						vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
						submitInfo.waitSemaphoreCount = 1;
						submitInfo.pWaitSemaphores = &swap.render;
						submitInfo.pWaitDstStageMask = waitStages;
						submitInfo.commandBufferCount = 1;
						submitInfo.pCommandBuffers = &commandBuffer;
						submitInfo.signalSemaphoreCount = 1;
						submitInfo.pSignalSemaphores = &swap.present;
						vuk::g_info.device.resetFences(swap.inFlight);
						vuk::g_info.queues.graphics.front().submit(submitInfo, swap.inFlight);

						pSwapchain->present(swap.present);
					};

					if (w0.isOpen())
					{
						auto const& swap0 = swaps0.at(frameIdx);
						drawFrame(WindowImpl::swapchain(w0.id()), static_cast<vk::Pipeline>(pipeline0), swap0);
					}
					if (w1.isOpen())
					{
						auto const& swap1 = swaps1.at(frameIdx);
						drawFrame(WindowImpl::swapchain(w1.id()), static_cast<vk::Pipeline>(pipeline1), swap1);
					}
					frameIdx = (frameIdx + 1) % maxFrames;
				}
				catch (std::exception const& e)
				{
					LOG_E("EXCEPTION!\n\t{}", e.what());
				}
			}
			vuk::g_info.device.waitIdle();
			vuk::vkDestroy(pipeline0, pipeline1);
			vuk::vkDestroy(pipelineLayout);
		}
		for (auto const& swap : swaps0)
		{
			vuk::g_info.device.destroy(swap.present);
			vuk::g_info.device.destroy(swap.render);
			vuk::g_info.device.destroy(swap.inFlight);
		}
		for (auto const& swap : swaps1)
		{
			vuk::g_info.device.destroy(swap.present);
			vuk::g_info.device.destroy(swap.render);
			vuk::g_info.device.destroy(swap.inFlight);
		}
		vuk::g_info.device.destroy(commandPool);
	}
	catch (std::exception const& e)
	{
		LOG_E("Exception!\n\t{}", e.what());
	}
	std::this_thread::sleep_for(stdch::milliseconds(maths::randomRange(10, 1000)));
	return 0;
}
} // namespace le
