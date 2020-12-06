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
		auto skyV = graphics::utils::compileGlsl("shaders/skybox.vert", {}, prefix);
		auto skyF = graphics::utils::compileGlsl("shaders/skybox.frag", {}, prefix);
		auto vert = reader.bytes("shaders/uber.vert.spv");
		auto frag = reader.bytes("shaders/uber.frag.spv");
		auto tex0 = reader.bytes("textures/container2.png");
		std::vector<bytearray> cubemap;
		cubemap.push_back(*reader.bytes("skyboxes/sky_dusk/right.jpg"));
		cubemap.push_back(*reader.bytes("skyboxes/sky_dusk/left.jpg"));
		cubemap.push_back(*reader.bytes("skyboxes/sky_dusk/up.jpg"));
		cubemap.push_back(*reader.bytes("skyboxes/sky_dusk/down.jpg"));
		cubemap.push_back(*reader.bytes("skyboxes/sky_dusk/front.jpg"));
		cubemap.push_back(*reader.bytes("skyboxes/sky_dusk/back.jpg"));
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
			auto const skyCubeI = gcube.indices;
			auto const skyCubeV = gcube.positions();
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
			graphics::Mesh skyCube("sky_cube", boot.vram, graphics::Mesh::Type::eStatic);
			vk::Sampler sampler = context.makeSampler(context.samplerInfo({vk::Filter::eLinear, vk::Filter::eLinear}));
			graphics::Texture texC("container", boot.vram), texR("red", boot.vram), sky("sky_dusk", boot.vram);
			graphics::Texture::CreateInfo texInfo;
			graphics::Texture::Raw raw;
			raw.bytes = {0xff, 0, 0, 0xff};
			raw.size = {1, 1};
			graphics::Texture::Compressed comp = {{std::move(*tex0)}};
			graphics::Texture::Compressed cm = {std::move(cubemap)};
			texInfo.data = std::move(comp);
			texInfo.sampler = sampler;
			texC.construct(texInfo);
			texInfo.data = raw;
			texR.construct(texInfo);
			texInfo.data = std::move(cm);
			sky.construct(texInfo);
			mesh0.construct(gcube);
			mesh1.construct(gcone);
			skyCube.construct(Span(skyCubeV), skyCubeI);
			if (!testV || !testF || !testFTex || !skyV || !skyF) {
				logE("shaders missing");
				return 1;
			}
			auto test = graphics::Shader(boot.device, {*reader.bytes(*testV), *reader.bytes(*testF)});
			auto testTex = graphics::Shader(boot.device, {*reader.bytes(*testV), *reader.bytes(*testFTex)});
			auto sskybox = graphics::Shader(boot.device, {*reader.bytes(*skyV), *reader.bytes(*skyF)});
			auto pipe = context.makePipeline("test", context.pipeInfo(test));
			auto pipeTex = context.makePipeline("test_tex", context.pipeInfo(testTex));
			struct Sets {
				std::unordered_map<u32, graphics::SetFactory> sets;

				void make(Span<u32> setNumbers, graphics::Pipeline const& pipe) {
					for (u32 num : setNumbers) {
						sets.emplace(num, pipe.makeSetFactory(num));
					}
				}

				graphics::SetFactory& operator[](u32 set) {
					if (auto it = sets.find(set); it != sets.end()) {
						return it->second;
					}
					ENSURE(false, "Nonexistent set");
					throw std::runtime_error("Nonexistent set");
				}

				void swap() {
					for (auto& [_, set] : sets) {
						set.swap();
					}
				}
			};
			struct SetLayouts {
				std::unordered_map<Hash, Sets> sets;

				void make(Hash layout, Span<u32> setNumbers, graphics::Pipeline const& pipe) {
					sets[layout].make(setNumbers, pipe);
				}

				Sets& operator[](Hash hash) {
					if (auto it = sets.find(hash); it != sets.end()) {
						return it->second;
					}
					ENSURE(false, "Nonexistent layout");
					throw std::runtime_error("Nonexistent layout");
				}

				void swap() {
					for (auto& [_, s] : sets) {
						s.swap();
					}
				}
			};
			SetLayouts layouts;
			std::array const setNums = {0U, 1U, 2U};
			layouts.make("main", setNums, pipeTex);
			auto pipeSkyInfo = context.pipeInfo(sskybox);
			pipeSkyInfo.fixedState.depthStencilState.depthWriteEnable = false;
			pipeSkyInfo.fixedState.vertexInput = context.vertexInput({0, sizeof(glm::vec3), {}});
			auto pipeSky = context.makePipeline("skybox", pipeSkyInfo);
			layouts.make("skybox", 0, pipeSky);
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
					virtual void write(graphics::DescriptorSet&) {
					}
					virtual void bind(graphics::CommandBuffer&, graphics::Pipeline const&, graphics::DescriptorSet const&) const {
					}
				};

				struct TexturedMaterial : Material {
					CView<graphics::Texture> diffuse;
					u32 binding = 0;

					void write(graphics::DescriptorSet& ds) override {
						ENSURE(diffuse, "Null pipeline/texture view");
						ds.updateTextures(binding, *diffuse);
					}
					void bind(graphics::CommandBuffer& cb, graphics::Pipeline const& pi, graphics::DescriptorSet const& ds) const override {
						ENSURE(diffuse, "Null texture view");
						cb.bindSets(pi.layout(), ds.get(), ds.setNumber());
					}
				};

				struct Prop {
					Ref<Transform> transform;
					Ref<graphics::Mesh> mesh;
					Ref<Material> material;
				};
				struct Skybox {
					CView<graphics::Mesh> mesh;
					CView<graphics::Texture> cubemap;

					bool ready() const {
						return cubemap && cubemap->ready();
					}
					void update(graphics::DescriptorSet& set, CView<graphics::Buffer> vp) const {
						if (ready()) {
							set.updateBuffers(0, vp, sizeof(VP));
							set.updateTextures(1, *cubemap);
						}
					}
					void draw(graphics::CommandBuffer& cb, graphics::Pipeline const& pi, graphics::DescriptorSet const& set) {
						if (ready()) {
							cb.bindPipe(pi);
							cb.bindSets(pi.layout(), set.get(), set.setNumber());
							cb.bindVBO(mesh->vbo().buffer, mesh->ibo().buffer);
							cb.drawIndexed(mesh->ibo().count);
						}
					}
				};
				struct Scene {
					Skybox skybox;
					std::unordered_map<Ref<graphics::Pipeline>, std::vector<Prop>> props;
				};
				TexturedMaterial texMat;
				texMat.diffuse = texC;
				Material mat;
				Scene scene;
				scene.skybox.mesh = skyCube;
				scene.skybox.cubemap = sky;
				scene.props[*pipeTex].push_back(Prop{tr[0], mesh0, texMat});
				scene.props[*pipe].push_back(Prop{tr[1], mesh0, mat});
				scene.props[*pipe].push_back(Prop{tr[2], mesh1, mat});

				// render
				if (context.waitForFrame()) {
					// write / update
					auto& smain = layouts["main"];
					auto& ssky = layouts["skybox"];
					smain[0].front().writeBuffer(0, vp);
					scene.skybox.update(ssky[0].front(), smain[0].front().buffers(0).front());
					std::size_t idx = 0;
					for (auto& [p, props] : scene.props) {
						for (auto& prop : props) {
							Material& mat = prop.material;
							Transform& t = prop.transform;
							smain[1].at(idx).writeBuffer(0, t.model());
							mat.write(smain[2].at(idx));
							++idx;
						}
					}
					// draw
					if (auto r = context.render(Colour(0x040404ff))) {
						auto& cb = r->primary();
						cb.setViewportScissor(context.viewport(), context.scissor());
						scene.skybox.draw(cb, pipeSky, ssky[0].front());
						cb.bindSets(pipe->layout(), smain[0].front().get(), smain[0].front().setNumber());
						std::size_t idx = 0;
						for (auto& [p, props] : scene.props) {
							graphics::Pipeline& pi = p;
							cb.bindPipe(pi);
							for (auto const& prop : props) {
								Material& mat = prop.material;
								graphics::Mesh& m = prop.mesh;
								cb.bindSets(pipe->layout(), smain[1].at(idx).get(), smain[1].at(idx).setNumber());
								mat.bind(cb, pi, smain[2].at(idx));
								cb.bindVBO(m.vbo().buffer, m.ibo().buffer);
								if (m.hasIndices()) {
									cb.drawIndexed(m.ibo().count);
								} else {
									cb.draw(m.vbo().count);
								}
								++idx;
							}
						}
						layouts.swap();
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
