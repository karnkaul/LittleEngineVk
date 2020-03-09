#include <glm/gtx/quaternion.hpp>
#include "core/io.hpp"
#include "core/jobs.hpp"
#include "core/log.hpp"
#include "core/maths.hpp"
#include "core/map_store.hpp"
#include "core/services.hpp"
#include "engine/levk.hpp"
#include "engine/window/window.hpp"
#include "engine/vuk/context/swapchain.hpp"
#include "engine/vuk/instance/instance.hpp"
#include "engine/vuk/pipeline/pipeline.hpp"
#include "engine/vuk/pipeline/shader.hpp"
#include "vuk/instance/instance_impl.hpp"

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
		services.add<vuk::Instance::Service>();
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
		auto device = static_cast<vk::Device>(*vuk::g_pDevice);
		vuk::Shader::Data tutorialData;
		tutorialData.pReader = g_uReader.get();
		tutorialData.codeIDMap[vuk::Shader::Type::eVertex] = "shaders/tutorial.vert.spv";
		tutorialData.codeIDMap[vuk::Shader::Type::eFragment] = "shaders/tutorial.frag.spv";
		vuk::Shader tutorialShader(std::move(tutorialData));

		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.queueFamilyIndex = vuk::g_pDevice->m_queueFamilyIndices.graphics.value();
		vk::CommandPool commandPool = device.createCommandPool(commandPoolCreateInfo);

		Window w0, w1;
		Window::Data data0;
		data0.size = {1280, 720};
		data0.title = "LittleEngineVk Demo";
		auto data1 = data0;
		data1.title += " 2";
		data1.position = {100, 100};
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

		if (w0.create(data0) && w1.create(data1))
		{
			vuk::Pipeline::Data pipelineData;
			auto pSwapchain = w0.swapchain();
			pipelineData.pShader = &tutorialShader;
			pipelineData.renderPass = pSwapchain->m_defaultRenderPass;
			pipelineData.viewport.width = (f32)pSwapchain->m_extent.width;
			pipelineData.viewport.height = (f32)pSwapchain->m_extent.height;
			pipelineData.viewport.minDepth = 0.0f;
			pipelineData.viewport.maxDepth = 1.0f;
			pipelineData.scissor.offset = vk::Offset2D(0, 0);
			pipelineData.scissor.extent = pSwapchain->m_extent;
			vuk::Pipeline pipeline(pipelineData);

			vk::CommandBufferAllocateInfo allocInfo;
			allocInfo.commandPool = commandPool;
			allocInfo.level = vk::CommandBufferLevel::ePrimary;
			allocInfo.commandBufferCount = (u32)pSwapchain->m_framebuffers.size();
			size_t idx = 0;
			std::vector<vk::CommandBuffer> commandBuffers = device.allocateCommandBuffers(allocInfo);
			for (auto const& commandBuffer : commandBuffers)
			{
				vk::CommandBufferBeginInfo beginInfo;
				commandBuffer.begin(beginInfo);
				vk::RenderPassBeginInfo renderPassInfo;
				renderPassInfo.renderPass = pSwapchain->m_defaultRenderPass;
				renderPassInfo.framebuffer = pSwapchain->m_framebuffers.at(idx);
				renderPassInfo.renderArea.extent = pSwapchain->m_extent;
				vk::ClearValue clearValue(0.0f);
				renderPassInfo.clearValueCount = 1;
				renderPassInfo.pClearValues = &clearValue;
				commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

				commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, static_cast<vk::Pipeline>(pipeline));
				commandBuffer.draw(3, 1, 0, 0);

				commandBuffer.endRenderPass();
				commandBuffer.end();
				++idx;
			}

			while (w0.isOpen() || w1.isOpen())
			{
				Window::pollEvents();
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
			}
			device.waitIdle();
		}
		device.destroy(commandPool);
	}
	catch (std::exception const& e)
	{
		LOG_E("Exception!\n\t{}", e.what());
	}
	std::this_thread::sleep_for(stdch::milliseconds(maths::randomRange(10, 1000)));
	return 0;
}
} // namespace le
