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
#include "vuk/info.hpp"
#include "vuk/presenter.hpp"
#include "vuk/renderer.hpp"
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

		vuk::Vertex const triangle0Verts[] = {
			vuk::Vertex{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			vuk::Vertex{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			vuk::Vertex{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		};

		vuk::Vertex const quad0Verts[] = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
										  {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
										  {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
										  {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
		u32 const quad0Indices[] = {0, 1, 2, 2, 3, 0};

		auto createStagingBuffer = [](vk::DeviceSize size) -> vuk::VkResource<vk::Buffer> {
			vuk::BufferData data;
			data.size = size;
			data.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			data.usage = vk::BufferUsageFlagBits::eTransferSrc;
			return vuk::createBuffer(data);
		};
		auto createDeviceBuffer = [](vk::DeviceSize size, vk::BufferUsageFlags flags) -> vuk::VkResource<vk::Buffer> {
			vuk::BufferData data;
			data.size = size;
			data.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
			data.usage = flags | vk::BufferUsageFlagBits::eTransferDst;
			return vuk::createBuffer(data);
		};
		auto copyBuffer = [](vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, vk::Queue queue,
							 vk::CommandPool pool) -> vuk::TransferOp {
			vuk::TransferOp ret{queue, pool, {}, {}};
			vuk::copyBuffer(src, dst, size, &ret);
			return ret;
		};

		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		auto const& qfi = vuk::g_info.queueFamilyIndices;
		commandPoolCreateInfo.queueFamilyIndex = qfi.transfer;
		auto transferPool = vuk::g_info.device.createCommandPool(commandPoolCreateInfo);

		auto q = vuk::g_info.queues.transfer;
		vuk::VkResource<vk::Buffer> triangle0VB, quad0VBIB;
		{
			std::vector<vuk::TransferOp> ops;
			auto const t0vbSize = sizeof(triangle0Verts);
			auto const q0vbSize = sizeof(quad0Verts);
			auto const q0ibSize = sizeof(quad0Indices);
			auto const q0vbibSize = q0vbSize + q0ibSize;
			auto tri0stage = createStagingBuffer(t0vbSize);
			auto quad0stage = createStagingBuffer(q0vbibSize);
			triangle0VB = createDeviceBuffer(t0vbSize, vk::BufferUsageFlagBits::eVertexBuffer);
			quad0VBIB = createDeviceBuffer(q0vbibSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer);
			vuk::writeToBuffer(tri0stage, triangle0Verts);
			vuk::writeToBuffer(quad0stage, quad0Verts, q0vbSize);
			vuk::writeToBuffer(quad0stage, quad0Indices, q0ibSize, q0vbSize);
			ops.push_back(copyBuffer(tri0stage.resource, triangle0VB.resource, t0vbSize, q, transferPool));
			ops.push_back(copyBuffer(quad0stage.resource, quad0VBIB.resource, q0vbibSize, q, transferPool));
			std::vector<vk::Fence> fences;
			std::for_each(ops.begin(), ops.end(), [&fences](auto const& op) { fences.push_back(op.transferred); });
			vuk::waitAll(fences);
			for (auto& op : ops)
			{
				vuk::vkDestroy(op.transferred);
				vuk::g_info.device.freeCommandBuffers(op.pool, op.commandBuffer);
			}
			vuk::vkDestroy(tri0stage, quad0stage);
		}

		Window w0, w1;
		Window::Data data0;
		data0.config.size = {1280, 720};
		data0.config.title = "LittleEngineVk Demo";
		auto data1 = data0;
		// data1.config.mode = Window::Mode::eBorderlessFullscreen;
		data1.config.title += " 2";
		data1.config.centreOffset = {100, 100};
		data1.options.colourSpace = ColourSpace::eRGBLinear;
		bool bRecreate0 = false, bRecreate1 = false;
		bool bClose0 = false, bClose1 = false;
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
		std::shared_ptr<s32> token0, token1;
		registerInput(w0, w1, bRecreate1, bClose0, token0);
		registerInput(w1, w0, bRecreate0, bClose1, token1);
		auto createRenderer = [&tutorialShader](vk::Pipeline* pPipeline, vk::PipelineLayout* pLayout, vuk::Presenter** ppPresenter,
												WindowID id) {
			vuk::vkDestroy(*pPipeline, *pLayout);
			*ppPresenter = WindowImpl::presenter(id);
			vk::PipelineLayoutCreateInfo layoutCreateInfo;
			vk::DescriptorSetLayout setLayout = vuk::g_info.matricesLayout;
			layoutCreateInfo.setLayoutCount = 1;
			layoutCreateInfo.pSetLayouts = &setLayout;
			vk::PushConstantRange pushConstantRange;
			pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
			pushConstantRange.offset = 0;
			pushConstantRange.size = sizeof(glm::mat4);
			layoutCreateInfo.pushConstantRangeCount = 1;
			layoutCreateInfo.pPushConstantRanges = &pushConstantRange;
			*pLayout = vuk::g_info.device.createPipelineLayout(layoutCreateInfo);
			vuk::PipelineData pipelineData;
			pipelineData.pShader = &tutorialShader;
			pipelineData.renderPass = (*ppPresenter)->m_renderPass;
			*pPipeline = vuk::createPipeline(*pLayout, pipelineData);
			return std::make_unique<vuk::Renderer>(*ppPresenter, 2);
		};

		if (w0.create(data0) && w1.create(data1))
		{
			std::unique_ptr<vuk::Renderer> uRenderer0, uRenderer1;
			vuk::Presenter* pPresenter0 = nullptr;
			vuk::Presenter* pPresenter1 = nullptr;
			vk::PipelineLayout layout0, layout1;
			vk::Pipeline pipeline0;
			vk::Pipeline pipeline1;
			if (w0.isOpen())
			{
				uRenderer0 = createRenderer(&pipeline0, &layout0, &pPresenter0, w0.id());
			}
			if (w1.isOpen())
			{
				uRenderer1 = createRenderer(&pipeline1, &layout1, &pPresenter1, w1.id());
			}

			vuk::ubo::View view0;
			vuk::ubo::View view1;
			Transform transform0;

			Time t = Time::elapsed();
			while (w0.isOpen() || w1.isOpen())
			{
				[[maybe_unused]] Time dt = Time::elapsed() - t;
				t = Time::elapsed();

				{
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
					uRenderer0 = createRenderer(&pipeline0, &layout0, &pPresenter0, w0.id());
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
					auto drawFrame = [&transform0](vuk::ubo::View* pView, vuk::Renderer* pRenderer, vk::Pipeline pipeline,
												   vk::PipelineLayout layout, vk::Buffer vertexBuffer, u32 vertCount,
												   u32 indexCount) -> bool {
						vuk::ClearValues clear;
						clear.colour = Colour(0x030203ff);
						auto write = [&](vuk::Renderer::FrameDriver::UBOs const& ubos) {
							// Set UBOs
							vuk::writeToBuffer(ubos.view.buffer, pView);
						};
						auto draw = [&](vuk::Renderer::FrameDriver const& driver) {
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
								vk::DeviceSize offset = vertCount * sizeof(vuk::Vertex);
								driver.commandBuffer.bindIndexBuffer(vertexBuffer, offset, vk::IndexType::eUint32);
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
						drawFrame(&view0, uRenderer0.get(), pipeline0, layout0, quad0VBIB.resource, (u32)arraySize(quad0Verts),
								  (u32)arraySize(quad0Indices));
					}
					if (w1.isOpen() && uRenderer1.get())
					{
						drawFrame(&view1, uRenderer1.get(), pipeline1, layout1, triangle0VB.resource, (u32)arraySize(triangle0Verts), 0);
						// drawFrame(&view1, pPresenter1, pipeline1, layout1, quad0VBIB.resource, (u32)arraySize(quad0Verts),
						//	  (u32)arraySize(quad0Indices));
					}
				}
				catch (std::exception const& e)
				{
					LOG_E("EXCEPTION!\n\t{}", e.what());
				}
			}
			vuk::g_info.device.waitIdle();
			vuk::vkDestroy(pipeline0, pipeline1, layout0, layout1);
			vuk::vkDestroy(transferPool);
			vuk::g_info.device.destroyBuffer(triangle0VB.resource);
			vuk::g_info.device.destroyBuffer(quad0VBIB.resource);
			vuk::g_info.device.freeMemory(triangle0VB.memory);
			vuk::g_info.device.freeMemory(quad0VBIB.memory);
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
