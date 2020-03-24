#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
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
#include "gfx/draw/shader.hpp"
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
		gfx::InitData initData;
#if 1 || defined(LEVK_DEBUG) // Always enabled, for time being
		initData.options.flags.set(gfx::InitData::Flag::eValidation);
#endif
		initData.config.instanceExtensions = WindowImpl::vulkanInstanceExtensions();
		initData.config.createTempSurface = [&](vk::Instance instance) { return WindowImpl::createSurface(instance, dummyWindow); };

		services.add<gfx::Service>(std::move(initData));
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

		gfx::Shader::Data tutorialShaderData;
		tutorialShaderData.id = "shaders/tutorial";
		std::array shaderIDs = {stdfs::path("shaders/tutorial.vert"), stdfs::path("shaders/tutorial.frag")};
		ASSERT(g_uReader->checkPresences(ArrayView<stdfs::path const>(shaderIDs)), "Shaders missing!");
		tutorialShaderData.pReader = g_uReader.get();
		tutorialShaderData.codeIDMap[gfx::Shader::Type::eVertex] = shaderIDs.at(0);
		tutorialShaderData.codeIDMap[gfx::Shader::Type::eFragment] = shaderIDs.at(1);
		gfx::Shader tutorialShader(tutorialShaderData);

		gfx::Vertex const triangle0Verts[] = {
			gfx::Vertex{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			gfx::Vertex{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			gfx::Vertex{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		};

		gfx::Vertex const quad0Verts[] = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
										  {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
										  {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
										  {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
		u32 const quad0Indices[] = {0, 1, 2, 2, 3, 0};

		auto createStagingBuffer = [](vk::DeviceSize size) -> gfx::Buffer {
			gfx::BufferData data;
			data.size = size;
			data.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			data.usage = vk::BufferUsageFlagBits::eTransferSrc;
			data.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY;
			return gfx::vram::createBuffer(data);
		};
		auto createDeviceBuffer = [](vk::DeviceSize size, vk::BufferUsageFlags flags) -> gfx::Buffer {
			gfx::BufferData data;
			data.size = size;
			data.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
			data.usage = flags | vk::BufferUsageFlagBits::eTransferDst;
			data.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
			return gfx::vram::createBuffer(data);
		};
		auto copyBuffer = [](vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, vk::Queue queue,
							 vk::CommandPool pool) -> gfx::TransferOp {
			gfx::TransferOp ret{queue, pool, {}, {}};
			gfx::vram::copy(src, dst, size, &ret);
			return ret;
		};

		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		auto const& qfi = gfx::g_info.queueFamilyIndices;
		commandPoolCreateInfo.queueFamilyIndex = qfi.transfer;
		auto transferPool = gfx::g_info.device.createCommandPool(commandPoolCreateInfo);

		vk::DescriptorSetLayout viewSetLayout = gfx::createDescriptorSetLayout(0, 1, vk::ShaderStageFlagBits::eVertex);

		auto q = gfx::g_info.queues.transfer;
		gfx::Buffer triangle0VB, quad0VB, quad0IB;
		{
			std::vector<gfx::TransferOp> ops;
			auto const t0vbSize = sizeof(triangle0Verts);
			auto const q0vbSize = sizeof(quad0Verts);
			auto const q0ibSize = sizeof(quad0Indices);
			auto tri0stage = createStagingBuffer(t0vbSize);
			auto quad0vstage = createStagingBuffer(q0vbSize);
			auto quad0istage = createStagingBuffer(q0ibSize);
			triangle0VB = createDeviceBuffer(t0vbSize, vk::BufferUsageFlagBits::eVertexBuffer);
			quad0VB = createDeviceBuffer(q0vbSize, vk::BufferUsageFlagBits::eVertexBuffer);
			quad0IB = createDeviceBuffer(q0ibSize, vk::BufferUsageFlagBits::eIndexBuffer);
			gfx::vram::write(tri0stage, triangle0Verts);
			gfx::vram::write(quad0vstage, quad0Verts);
			gfx::vram::write(quad0istage, quad0Indices);
			ops.push_back(copyBuffer(tri0stage.buffer, triangle0VB.buffer, t0vbSize, q, transferPool));
			ops.push_back(copyBuffer(quad0vstage.buffer, quad0VB.buffer, q0vbSize, q, transferPool));
			ops.push_back(copyBuffer(quad0istage.buffer, quad0IB.buffer, q0ibSize, q, transferPool));
			std::vector<vk::Fence> fences;
			std::for_each(ops.begin(), ops.end(), [&fences](auto const& op) { fences.push_back(op.transferred); });
			gfx::waitAll(fences);
			for (auto& op : ops)
			{
				gfx::vkDestroy(op.transferred);
				gfx::g_info.device.freeCommandBuffers(op.pool, op.commandBuffer);
			}
			gfx::vram::release(tri0stage, quad0vstage, quad0istage);
		}

		Window w0, w1;
		Window::Data data0;
		data0.config.size = {1280, 720};
		data0.config.title = "LittleEngineVk Demo";
		auto data1 = data0;
		// data1.config.mode = Window::Mode::eBorderlessFullscreen;
		data1.config.title += " 2";
		data1.config.centreOffset = {100, 100};
		data1.options.colourSpaces = {ColourSpace::eRGBLinear};
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
		auto createPipeline = [&tutorialShader](vk::Pipeline* pPipeline, vk::PipelineLayout layout, vk::RenderPass renderPass) {
			gfx::PipelineData pipelineData;
			pipelineData.pShader = &tutorialShader;
			pipelineData.renderPass = renderPass;
			*pPipeline = gfx::createPipeline(layout, pipelineData);
		};
		auto createRenderer = [&viewSetLayout, &createPipeline](vk::Pipeline* pPipeline, vk::PipelineLayout* pLayout,
																gfx::Presenter** ppPresenter, WindowID id) {
			gfx::vkDestroy(*pPipeline, *pLayout);
			*ppPresenter = WindowImpl::presenter(id);
			vk::PipelineLayoutCreateInfo layoutCreateInfo;
			layoutCreateInfo.setLayoutCount = 1;
			layoutCreateInfo.pSetLayouts = &viewSetLayout;
			vk::PushConstantRange pushConstantRange;
			pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
			pushConstantRange.offset = 0;
			pushConstantRange.size = sizeof(glm::mat4);
			layoutCreateInfo.pushConstantRangeCount = 1;
			layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
			*pLayout = gfx::g_info.device.createPipelineLayout(layoutCreateInfo);
			createPipeline(pPipeline, *pLayout, (*ppPresenter)->m_renderPass);
			gfx::Renderer::Data data;
			data.frameCount = 2;
			data.pPresenter = *ppPresenter;
			data.uboSetLayouts.view = viewSetLayout;
			return std::make_unique<gfx::Renderer>(data);
		};
		if (w0.create(data0) && w1.create(data1))
		{
			std::unique_ptr<gfx::Renderer> uRenderer0, uRenderer1;
			gfx::Presenter* pPresenter0 = nullptr;
			gfx::Presenter* pPresenter1 = nullptr;
			vk::PipelineLayout layout0, layout1;
			vk::Pipeline pipeline0, pipeline0wf;
			vk::Pipeline pipeline1;
			if (w0.isOpen())
			{
				gfx::vkDestroy(pipeline0wf);
				uRenderer0 = createRenderer(&pipeline0, &layout0, &pPresenter0, w0.id());
				gfx::PipelineData pipelineData;
				pipelineData.pShader = &tutorialShader;
				pipelineData.renderPass = pPresenter0->m_renderPass;
				pipelineData.polygonMode = vk::PolygonMode::eLine;
				pipelineData.staticLineWidth = gfx::g_info.lineWidth(3.0f);
				pipeline0wf = gfx::createPipeline(layout0, pipelineData);
			}
			if (w1.isOpen())
			{
				uRenderer1 = createRenderer(&pipeline1, &layout1, &pPresenter1, w1.id());
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
#if defined(LEVK_SHADER_HOT_RELOAD)
					if (tutorialShader.hasReloaded() == FileMonitor::Status::eModified)
					{
						gfx::g_info.device.waitIdle();
						gfx::vkDestroy(pipeline0, pipeline1, pipeline0wf);
						createPipeline(&pipeline0, layout0, WindowImpl::presenter(w0.id())->m_renderPass);
						createPipeline(&pipeline1, layout1, WindowImpl::presenter(w1.id())->m_renderPass);
						gfx::PipelineData pipelineData;
						pipelineData.pShader = &tutorialShader;
						pipelineData.renderPass = WindowImpl::presenter(w0.id())->m_renderPass;
						pipelineData.polygonMode = vk::PolygonMode::eLine;
						pipelineData.staticLineWidth = gfx::g_info.lineWidth(3.0f);
						pipeline0wf = gfx::createPipeline(layout0, pipelineData);
					}
#endif
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
					gfx::vkDestroy(pipeline0wf);
					uRenderer0 = createRenderer(&pipeline0, &layout0, &pPresenter0, w0.id());
					gfx::PipelineData pipelineData;
					pipelineData.pShader = &tutorialShader;
					pipelineData.renderPass = pPresenter0->m_renderPass;
					pipelineData.polygonMode = vk::PolygonMode::eLine;
					pipelineData.staticLineWidth = gfx::g_info.lineWidth(3.0f);
					pipeline0wf = gfx::createPipeline(layout0, pipelineData);
				}
				if (bRecreate1)
				{
					bRecreate1 = false;
					w1.create(data1);
					uRenderer1 = createRenderer(&pipeline1, &layout1, &pPresenter1, w1.id());
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
					auto drawFrame = [&transform0](gfx::ubo::View* pView, gfx::Renderer* pRenderer, vk::Pipeline pipeline,
												   vk::PipelineLayout layout, vk::Buffer vertexBuffer, vk::Buffer indexBuffer,
												   u32 vertCount, u32 indexCount) -> bool {
						gfx::ClearValues clear;
						clear.colour = Colour(0x030203ff);
						auto write = [&](gfx::ubo::UBOs const& ubos) { ubos.view.write(*pView); };
						auto draw = [&](gfx::Renderer::FrameDriver const& driver) {
							vk::Viewport viewport = pRenderer->transformViewport();
							vk::Rect2D scissor = pRenderer->transformScissor();
							driver.commandBuffer.setViewport(0, viewport);
							driver.commandBuffer.setScissor(0, scissor);
							driver.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
							vk::DeviceSize offsets[] = {0};
							driver.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0,
																	driver.ubos.view.descriptorSet, {});
							driver.commandBuffer.pushConstants(layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4),
															   glm::value_ptr(transform0.model()));
							driver.commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);
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
						drawFrame(&view0, uRenderer0.get(), bWF0 ? pipeline0wf : pipeline0, layout0, quad0VB.buffer, quad0IB.buffer,
								  (u32)arraySize(quad0Verts), (u32)arraySize(quad0Indices));
					}
					if (w1.isOpen() && uRenderer1.get())
					{
						drawFrame(&view1, uRenderer1.get(), pipeline1, layout1, triangle0VB.buffer, {}, (u32)arraySize(triangle0Verts), 0);
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
			gfx::vkDestroy(pipeline0, pipeline0wf, pipeline1, layout0, layout1);
			gfx::vkDestroy(transferPool);
			gfx::vkDestroy(viewSetLayout);
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
