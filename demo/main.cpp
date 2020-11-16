#include <engine/levk.hpp>
#include <level.hpp>

#include <core/index_view.hpp>
#include <graphics/context/bootstrap.hpp>
#include <graphics/geometry.hpp>
#include <graphics/mesh.hpp>
#include <graphics/render_context.hpp>
#include <graphics/shader.hpp>
#include <graphics/texture.hpp>
#include <graphics/utils/utils.hpp>
#include <window/desktop_instance.hpp>

using namespace le;

enum class Flag { eRecreated, eResized, ePaused, eClosed, eDebug0, eCOUNT_ };
using Flags = kt::enum_flags<Flag>;

static void poll(Flags& out_flags, window::EventQueue queue) {
	while (auto e = queue.pop()) {
		switch (e->type) {
		case window::Event::Type::eClose: {
			out_flags.set(Flag::eClosed);
			break;
		}
		case window::Event::Type::eInput: {
			auto const& input = e->payload.input;
			if (input.key == window::Key::eW && input.action == window::Action::eRelease && input.mods[window::Mod::eControl]) {
				out_flags.set(Flag::eClosed);
			}
			if (input.key == window::Key::eD && input.action == window::Action::eRelease && input.mods[window::Mod::eControl]) {
				out_flags.flip(Flag::eDebug0);
			}
			break;
		}
		case window::Event::Type::eSuspend: {
			out_flags[Flag::ePaused] = e->payload.bSet;
			break;
		}
		case window::Event::Type::eResize: {
			auto const& resize = e->payload.resize;
			if (resize.bFramebuffer) {
				out_flags.set(Flag::eResized);
			}
			break;
		}
		default:
			break;
		}
	}
}

int main(int argc, char** argv) {
	try {
		tasks::Service ts(4);
		io::FileReader reader;
		io::Path const prefix = os::dirPath(os::Dir::eWorking) / "data";
		reader.mount(prefix);
		reader.mount(os::dirPath(os::Dir::eWorking) / "demo/data");
		auto testV = graphics::utils::compileGlsl("shaders/test.vert", {}, prefix);
		auto testF = graphics::utils::compileGlsl("shaders/test.frag", {}, prefix);
		auto testFTex = graphics::utils::compileGlsl("shaders/test_tex.frag", {}, prefix);
		auto vert = reader.bytes("shaders/uber.vert.spv");
		auto frag = reader.bytes("shaders/uber.frag.spv");
		auto tex0 = reader.bytes("textures/container2.png");
		window::CreateInfo winInfo;
		winInfo.config.title = "levk demo";
		winInfo.config.size = {1280, 720};
		winInfo.options.bCentreCursor = true;
		window::DesktopInstance winst(winInfo);
		auto makeSurface = [&winst](vk::Instance instance) -> vk::SurfaceKHR {
			vk::SurfaceKHR surface;
			winst.vkCreateSurface(instance, surface);
			return surface;
		};
		graphics::Bootstrap::CreateInfo bootInfo;
		bootInfo.instance.extensions = winst.vkInstanceExtensions();
		bootInfo.instance.bValidation = true;
		bootInfo.instance.validationLog = dl::level::info;
		bootInfo.device.bPrintAvailable = true;
		graphics::Bootstrap boot(bootInfo, makeSurface, winst.framebufferSize());
		boot.vram.m_bLogAllocs = true;
		graphics::RenderContext context(boot.swapchain);
		{
			graphics::Shader shader(boot.device);
			if (vert && frag) {
				shader.reconstruct({std::move(*vert), std::move(*frag)});
				logD("uber shader created");
			}

			graphics::Geometry gcube = graphics::makeCube(0.5f);
			graphics::Geometry gcone = graphics::makeCone();
			for (auto& v : gcone.vertices) {
				v.colour = colours::yellow.toVec4();
			}
			struct VP {
				glm::mat4 mat_p;
				glm::mat4 mat_v;
			};
			VP vp = {glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f), glm::lookAt({1.0f, 1.0f, 1.0f}, {}, graphics::g_nUp)};
			graphics::Mesh mesh0("cube", boot.vram, graphics::Mesh::Type::eStatic);
			graphics::Mesh mesh1("cone", boot.vram, graphics::Mesh::Type::eStatic);
			vk::Sampler sampler = context.makeSampler(context.samplerInfo({vk::Filter::eLinear, vk::Filter::eLinear}));
			graphics::Texture texC("container", boot.vram), texR("red", boot.vram);
			graphics::Texture::CreateInfo texInfo;
			graphics::Texture::Raw raw;
			raw.bytes = {0xff, 0, 0, 0xff};
			raw.size = {1, 1};
			graphics::Texture::Compressed comp = {{std::move(*tex0)}};
			texInfo.data = std::move(comp);
			texInfo.sampler = sampler;
			texC.construct(texInfo);
			texInfo.data = raw;
			texR.construct(texInfo);
			mesh0.construct(gcube);
			mesh1.construct(gcone);
			if (!testV || !testF || !testFTex) {
				logE("shaders missing");
				return 1;
			}
			auto test = graphics::Shader(boot.device, {*reader.bytes(*testV), *reader.bytes(*testF)});
			auto testTex = graphics::Shader(boot.device, {*reader.bytes(*testV), *reader.bytes(*testFTex)});
			auto pipe = context.makePipeline("test", context.pipeInfo(test));
			auto pipeTex = context.makePipeline("test_tex", context.pipeInfo(testTex));
			texC.wait();
			texR.wait();
			winst.show();
			Flags flags;
			std::array<Transform, 3> tr;
			tr[1].position({-5.0f, -1.0f, -2.0f});
			tr[2].position({0.0f, -2.0f, -3.0f});
			time::Point t = time::now();
			while (true) {
				Time_s dt = time::now() - t;
				t = time::now();
				poll(flags, winst.pollEvents());
				if (flags.test(Flag::eClosed)) {
					break;
				}
				if (flags.test(Flag::ePaused)) {
					continue;
				}
				if (flags.test(Flag::eResized)) {
					/*if (!context.recreated(winst.framebufferSize())) {
						ENSURE(false, "Swapchain failure");
					}*/
					flags.reset(Flag::eResized);
				}
				if (context.reconstructd(winst.framebufferSize())) {
					continue;
				}

				// tick
				threads::sleep(5ms);
				auto const fb = winst.framebufferSize();
				vp.mat_p = glm::perspective(glm::radians(45.0f), (f32)fb.x / std::max((f32)fb.y, 1.0f), 0.1f, 100.0f);
				tr[0].rotate(glm::radians(-180.0f) * dt.count(), graphics::g_nUp);
				tr[1].rotate(glm::radians(360.0f) * dt.count(), graphics::g_nUp);

				struct Material {
					View<graphics::Pipeline> pipeline;

					virtual void write(std::size_t) {
					}
					virtual void bind(graphics::CommandBuffer&, std::size_t) {
					}
				};

				struct TexturedMaterial : Material {
					CView<graphics::Texture> diffuse;

					void write(std::size_t index) {
						ENSURE(pipeline && diffuse, "Null pipeline/texture view");
						pipeline->writeTextures({2, index}, 0, *diffuse);
					}
					void bind(graphics::CommandBuffer& cb, std::size_t index) {
						ENSURE(pipeline && diffuse, "Null pipeline/texture view");
						pipeline->bindSet(cb, {2, index});
					}
				};

				struct Prop {
					Ref<Transform> transform;
					Ref<graphics::Mesh> mesh;
					Ref<Material> material;

					void writeDescriptors(std::size_t index) {
						Material& mat = material;
						Transform& t = transform;
						ENSURE(mat.pipeline, "Null pipeline view");
						mat.pipeline->writeBuffer({1, index}, 0, t.model());
						mat.write(index);
					}

					void draw(graphics::CommandBuffer& cb, std::size_t idx) {
						Material& mat = material;
						graphics::Mesh& m = mesh;
						ENSURE(mat.pipeline, "Null pipeline view");
						mat.pipeline->bindSet(cb, {1, idx});
						mat.bind(cb, idx);
						cb.bindVBO(m.vbo().buffer, m.ibo().buffer);
						if (m.hasIndices()) {
							cb.drawIndexed(m.ibo().count);
						} else {
							cb.draw(m.vbo().count);
						}
					}
				};
				using Scene = std::unordered_map<Ref<graphics::Pipeline>, std::vector<Prop>>;
				TexturedMaterial texMat;
				texMat.pipeline = pipeTex;
				texMat.diffuse = texC;
				Material mat;
				mat.pipeline = pipe;
				Scene scene;
				scene[*texMat.pipeline].push_back(Prop{tr[0], mesh0, texMat});
				scene[*mat.pipeline].push_back(Prop{tr[1], mesh0, mat});
				scene[*mat.pipeline].push_back(Prop{tr[2], mesh1, mat});

				// render
				if (context.waitForFrame()) {
					// write
					for (auto& [p, props] : scene) {
						graphics::Pipeline& pipe = p;
						pipe.writeBuffer({0, 0}, 0, vp);
						for (auto [prop, idx] : IndexView(props)) {
							prop.writeDescriptors(idx);
						}
					}
					// draw
					if (auto r = context.render(Colour(0x040404ff))) {
						auto& cb = r->primary();
						cb.setViewportScissor(context.viewport(), context.scissor());
						for (auto& [p, props] : scene) {
							graphics::Pipeline& pipe = p;
							cb.bindPipe(pipe);
							pipe.bindSet(cb, {0, 0});
							for (auto [prop, idx] : IndexView(props)) {
								prop.draw(cb, idx);
							}
							pipe.swapSets();
						}
					}
				}

				flags.reset(Flag::eRecreated);
			}
		}
	} catch (std::exception const& e) {
		logE("exception: {}", e.what());
	}

	engine::Service engine({argc, argv});
	std::array<io::Path, 2> const pathSearch = {"data", "demo/data"};
	auto dataPaths = engine::locate(pathSearch);
	engine::Info info;
	Window::Info windowInfo;
	windowInfo.config.size = {1280, 720};
	windowInfo.config.title = "LittleEngineVk Demo";
	info.windowInfo = std::move(windowInfo);
	info.dataPaths = dataPaths;
#if defined(LEVK_DEBUG)
// info.bLogVRAMallocations = true;
#endif
	if (!engine.init(std::move(info))) {
		return 1;
	}
	engine::g_shutdownSequence = engine::ShutdownSequence::eShutdown_CloseWindow;
	while (engine.running()) {
		engine.update(g_driver);
		engine.render();
	}
	g_driver.cleanup();
	return 0;
}
