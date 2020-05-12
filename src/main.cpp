#include "core/assert.hpp"
#include "core/gdata.hpp"
#include "core/io.hpp"
#include "core/jobs.hpp"
#include "core/log.hpp"
#include "core/maths.hpp"
#include "core/map_store.hpp"
#include "core/transform.hpp"
#include "engine/levk.hpp"
#include "engine/assets/resources.hpp"
#include "engine/ecs/components.hpp"
#include "engine/ecs/registry.hpp"
#include "engine/gfx/camera.hpp"
#include "engine/gfx/font.hpp"
#include "engine/gfx/geometry.hpp"
#include "engine/gfx/light.hpp"
#include "engine/gfx/mesh.hpp"
#include "engine/gfx/model.hpp"
#include "engine/gfx/renderer.hpp"
#include "engine/gfx/shader.hpp"
#include "engine/gfx/texture.hpp"
#include "engine/window/window.hpp"
#include "tests/ecs_test.hpp"

using namespace le;

namespace
{
std::unique_ptr<IOReader> g_uReader;

bool runTests()
{
	if (test::ecs::run() != 0)
	{
		ASSERT(false, "ECS test failed!");
		return false;
	}
	return true;
}
} // namespace

int main(int argc, char** argv)
{
	engine::Service engine;
	if (!engine.start(argc, argv))
	{
		return 1;
	}
	if (!runTests())
	{
		return 1;
	}
	g_uReader = std::make_unique<FileReader>(engine::dataPath());
	gfx::Shader::Info tutorialShaderInfo;
	std::array shaderIDs = {stdfs::path("shaders/uber.vert"), stdfs::path("shaders/uber.frag")};
	ASSERT(g_uReader->checkPresences(ArrayView<stdfs::path const>(shaderIDs)), "Shaders missing!");
	tutorialShaderInfo.pReader = g_uReader.get();
	tutorialShaderInfo.codeIDMap.at((size_t)gfx::Shader::Type::eVertex) = shaderIDs.at(0);
	tutorialShaderInfo.codeIDMap.at((size_t)gfx::Shader::Type::eFragment) = shaderIDs.at(1);
	auto pShader = g_pResources->create<gfx::Shader>("shaders/uber", std::move(tutorialShaderInfo));

	gfx::Font::Info fontInfo;
	GData fontData(g_uReader->getString("fonts/default.json").payload);
	fontInfo.deserialise(fontData);
	fontInfo.image = g_uReader->getBytes(stdfs::path("fonts") / fontInfo.sheetID).payload;
	auto pFont = g_pResources->create<gfx::Font>("fonts/default", std::move(fontInfo));

	Registry registry;
	gfx::Mesh::Info triangle0info;
	// clang-format off
		triangle0info.geometry.vertices = {
			{{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {}, {0.5f, 0.0f}},
			{{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {}, {1.0f, 1.0f}},
			{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {0.0f, 1.0f}},
		};
	// clang-format on
	gfx::Mesh* pTriangle0 = g_pResources->create<gfx::Mesh>("meshes/triangle0", triangle0info);

	gfx::Mesh::Info meshInfo;
	meshInfo.geometry = gfx::createQuad();
	gfx::Mesh* pMesh0 = g_pResources->create<gfx::Mesh>("mesh0", meshInfo);
	meshInfo.geometry = gfx::createCubedSphere(1.0f, 8);
	gfx::Mesh* pMesh1 = g_pResources->create<gfx::Mesh>("mesh1", meshInfo);

	auto eid0 = registry.spawnEntity("entity0");
	auto eid1 = registry.spawnEntity("entity1");
	auto eid2 = registry.spawnEntity("entity2");
	auto eid3 = registry.spawnEntity("entity3");

	gfx::Model::LoadRequest model0lr;
	model0lr.jsonID = g_uReader->checkPresence("models/test/nanosuit/nanosuit.json") ? "models/test/nanosuit" : "models/plant";
	model0lr.pReader = g_uReader.get();
	stdfs::path model0id, model1id;
	gfx::Model* pModel0 = nullptr;
	gfx::Model* pModel1 = nullptr;
	jobs::enqueue([&pModel0, &model0lr, &model0id]() {
		auto model0info = gfx::Model::parseOBJ(model0lr);
		model0id = model0info.id;
		model0id += "_0";
		pModel0 = g_pResources->create<gfx::Model>(model0id, model0info);
	});
	jobs::enqueue([&pModel1, &model0lr, &model1id]() {
		auto model1info = gfx::Model::parseOBJ(model0lr);
		model1id = model1info.id;
		model1id += "_1";
		pModel1 = g_pResources->create<gfx::Model>(model1id, model1info);
	});
	gfx::Material::Info texturedInfo;
	texturedInfo.albedo.ambient = Colour(0x888888ff);
	auto pTexturedLit = g_pResources->create<gfx::Material>("materials/textured", texturedInfo);

	gfx::Texture::Info textureInfo;
	textureInfo.pReader = g_uReader.get();
	pMesh0->m_material.flags.set({gfx::Material::Flag::eTextured, gfx::Material::Flag::eLit, gfx::Material::Flag::eOpaque});
	pMesh0->m_material.pMaterial = pTexturedLit;
	pMesh1->m_material.flags.set({gfx::Material::Flag::eTextured, gfx::Material::Flag::eLit, gfx::Material::Flag::eOpaque});
	pMesh1->m_material.pMaterial = pTexturedLit;
	pMesh1->m_material.tint.a = 0xcc;
	textureInfo.assetID = "textures/container2.png";
	pMesh1->m_material.pDiffuse = pMesh0->m_material.pDiffuse = g_pResources->create<gfx::Texture>(textureInfo.assetID, textureInfo);
	textureInfo.assetID = "textures/container2_specular.png";
	pMesh1->m_material.pSpecular = pMesh0->m_material.pSpecular = g_pResources->create<gfx::Texture>(textureInfo.assetID, textureInfo);
	textureInfo.assetID = "textures/awesomeface.png";
	pMesh0->m_material.pDiffuse = g_pResources->create<gfx::Texture>(textureInfo.assetID, textureInfo);
	pMesh0->m_material.pSpecular = nullptr;
	pMesh0->m_material.flags.reset(gfx::Material::Flag::eOpaque);

	gfx::DirLight dirLight0;
	dirLight0.diffuse = Colour(0xffffffff);
	dirLight0.direction = glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f));
	gfx::DirLight dirLight1;
	dirLight1.diffuse = Colour(0xffffffff);
	dirLight1.direction = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

	gfx::Cubemap::Info cubemapInfo;
	cubemapInfo.pReader = g_uReader.get();
	stdfs::path const& cp = "skyboxes/sky_dusk";
	cubemapInfo.rludfbIDs = {cp / "right.jpg", cp / "left.jpg", cp / "up.jpg", cp / "down.jpg", cp / "front.jpg", cp / "back.jpg"};
	gfx::Cubemap* pCubemap = g_pResources->create<gfx::Cubemap>("skyboxes/sky_dusk_cubemap", cubemapInfo);
	gfx::Pipeline::Info skyboxPipeInfo;
	skyboxPipeInfo.name = "skybox";
	skyboxPipeInfo.pShader = pShader;
	skyboxPipeInfo.bDepthWrite = false;
	gfx::Pipeline* pSkyPipe = nullptr;

	Window w0, w1;
	Window::Info info0;
	info0.config.size = {1280, 720};
	info0.config.title = "LittleEngineVk Demo";
	info0.config.virtualFrameCount = 2;
	info0.config.centreOffset = {-200, -200};
	auto info1 = info0;
	// info1.config.mode = Window::Mode::eBorderlessFullscreen;
	info1.config.title += " 2";
	info1.config.centreOffset = {200, 200};
	bool bRecreate0 = false, bRecreate1 = false;
	bool bClose0 = false, bClose1 = false;
	bool bWF0 = false;
	bool bDisableCam = false;
	auto registerInput = [&bDisableCam](Window& self, Window& other, bool& bRecreate, bool& bClose, std::shared_ptr<int>& token) {
		token = self.registerInput([&](Key key, Action action, Mods mods) {
			if (self.isOpen() && key == Key::eW && action == Action::eRelease && mods & Mods::eCONTROL)
			{
				bClose = true;
			}
			if (!other.isOpen() && (key == Key::eT || key == Key::eN) && action == Action::eRelease && mods & Mods::eCONTROL)
			{
				bRecreate = true;
			}
			if (key == Key::eLeftControl || key == Key::eRightControl)
			{
				if (bDisableCam && action == Action::eRelease)
				{
					bDisableCam = false;
				}
				if (!bDisableCam && action == Action::ePress)
				{
					bDisableCam = true;
				}
			}
		});
	};
	std::shared_ptr<s32> token0, token1, wf0Token;
	bool bTEMP = false;
	bool bToggleModel0 = false;
	gfx::Model::Info m0info;
	gfx::Model::Info m1info;
	Time reloadTime;
	wf0Token = w0.registerInput([eid0, &registry, &bWF0, &bTEMP, &bToggleModel0](Key key, Action action, Mods mods) {
		if (key == Key::eP && action == Action::eRelease && (mods & Mods::eCONTROL))
		{
			bWF0 = !bWF0;
		}
		if (key == Key::eS && action == Action::eRelease && (mods & Mods::eCONTROL))
		{
			bTEMP = !bTEMP;
		}
		if (key == Key::eM && action == Action::eRelease && (mods & Mods::eCONTROL))
		{
			bToggleModel0 = true;
		}
		if (key == Key::eD && action == Action::eRelease && (mods & Mods::eCONTROL))
		{
			registry.destroyEntity(eid0);
		}
		if (key == Key::eE && action == Action::eRelease && (mods & Mods::eCONTROL))
		{
			registry.setEnabled(eid0, !registry.isEnabled(eid0));
		}
	});
	registerInput(w0, w1, bRecreate1, bClose0, token0);
	registerInput(w1, w0, bRecreate0, bClose1, token1);
	auto createPipeline = [pShader](gfx::Renderer* pRenderer, std::string_view name, gfx::PolygonMode mode = gfx::PolygonMode::eFill,
									f32 lineWidth = 3.0f) -> gfx::Pipeline* {
		gfx::Pipeline::Info pipelineInfo;
		pipelineInfo.pShader = pShader;
		pipelineInfo.name = name;
		pipelineInfo.polygonMode = mode;
		pipelineInfo.lineWidth = lineWidth;
		// pipelineInfo.cullMode = gfx::CullMode::eClockwise;
		pipelineInfo.bBlend = true;
		return pRenderer->createPipeline(std::move(pipelineInfo));
	};

	gfx::FreeCam freeCam0(&w0), freeCam1(&w1);
	freeCam0.m_state.flags.set(gfx::FreeCam::Flag::eKeyToggle_Look);
	freeCam1.m_state.flags = freeCam0.m_state.flags;

	gfx::Text2D fpsText, ftText;
	gfx::Text2D::Info textInfo;
	textInfo.data.colour = colours::white;
	textInfo.data.scale = 0.25f;
	textInfo.pFont = pFont;
	textInfo.pShader = pShader;
	textInfo.id = "fps";
	fpsText.setup(textInfo);
	textInfo.data.colour = colours::cyan;
	textInfo.data.pos.y -= 100.0f;
	textInfo.id = "ft";
	ftText.setup(textInfo);

	if (w1.create(info1) && w0.create(info0))
	{
		gfx::Pipeline *pPipeline0 = nullptr, *pPipeline0wf = nullptr, *pPipeline1 = nullptr;

		if (w0.isOpen())
		{
			auto& renderer0 = w0.renderer();
			pPipeline0 = createPipeline(&renderer0, "default");
			pPipeline0wf = createPipeline(&renderer0, "wireframe", gfx::PolygonMode::eLine);
			pSkyPipe = renderer0.createPipeline(skyboxPipeInfo);
		}
		if (w1.isOpen())
		{
			auto& renderer1 = w1.renderer();
			pPipeline1 = createPipeline(&renderer1, "default");
		}

		gfx::Renderer::View view0;
		gfx::Renderer::View view1;
		freeCam0.m_position = {0.0f, 1.0f, 2.0f};

		registry.addComponent<CData<Transform>>(eid0)->data.setPosition({1.0f, 1.0f, -2.0f});
		registry.addComponent<CResource<gfx::Mesh>>(eid0, pMesh0->m_id);

		registry.addComponent<CData<Transform>>(eid1)->data.setPosition({0.0f, 0.0f, -2.0f});
		registry.addComponent<CResource<gfx::Mesh>>(eid1, pMesh1->m_id);

		registry.addComponent<CData<Transform>>(eid2)->data.setPosition({-1.0f, 1.0f, -2.0f});
		registry.addComponent<CResource<gfx::Model>>(eid2, model0id);

		registry.addComponent<CData<Transform>>(eid3)->data.setPosition({0.0f, -1.0f, -3.0f});
		registry.addComponent<CResource<gfx::Model>>(eid3, model1id);

		Transform transform1;

		Time t = Time::elapsed();
		Time ft;
		while (w0.isOpen() || w1.isOpen())
		{
			Time dt = Time::elapsed() - t;
			t = Time::elapsed();
			Time fStart = Time::elapsed();

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
			{
				engine.update();
				registry.cleanDestroyed();

				if (bToggleModel0)
				{
					if (pModel0 && pModel1)
					{
						g_pResources->unload<gfx::Model>(pModel0->m_id);
						g_pResources->unload<gfx::Model>(pModel1->m_id);
						pModel0 = nullptr;
						pModel1 = nullptr;
					}
					else
					{
						reloadTime = Time::elapsed();
						jobs::enqueue(
							[&]() {
								m0info = gfx::Model::parseOBJ(model0lr);
								LOG_I("{} data loaded in: {}s", m0info.id.generic_string(), (Time::elapsed() - reloadTime).to_s());
								auto model0id = m0info.id;
								model0id += "_0";
								pModel0 = g_pResources->create<gfx::Model>(model0id, std::move(m0info));
								LOG_I("{} total load time: {}s", m0info.id.generic_string(), (Time::elapsed() - reloadTime).to_s());
							},
							"Model0-Reload");
						jobs::enqueue(
							[&]() {
								m1info = gfx::Model::parseOBJ(model0lr);
								LOG_I("{} data loaded in: {}s", m1info.id.generic_string(), (Time::elapsed() - reloadTime).to_s());
								auto model1id = m1info.id;
								model1id += "_1";
								pModel1 = g_pResources->create<gfx::Model>(model1id, std::move(m1info));
								LOG_I("{} total load time: {}s", m1info.id.generic_string(), (Time::elapsed() - reloadTime).to_s());
							},
							"Model1-Reload");
					}
					bToggleModel0 = false;
				}

				freeCam0.m_state.flags[gfx::FreeCam::Flag::eEnabled] = !bDisableCam;
				freeCam0.tick(dt);
				freeCam1.m_state.flags[gfx::FreeCam::Flag::eEnabled] = !bDisableCam;
				freeCam1.tick(dt);

				fpsText.updateText(fmt::format("{}FPS", fps == 0 ? frames : fps));
				ftText.updateText(fmt::format("{:.3}ms", ft.to_s() * 1000));

				if (auto pM = registry.component<CResource<gfx::Model>>(eid2))
				{
					pM->m_id = model0id;
				}
				if (auto pM = registry.component<CResource<gfx::Model>>(eid3))
				{
					pM->m_id = model1id;
				}

				// Update matrices
				if (auto pT = registry.component<CData<Transform>>(eid1))
				{
					pT->data.setOrientation(glm::rotate(pT->data.orientation(), glm::radians(dt.to_s() * 10), glm::vec3(0.0f, 1.0f, 0.0f)));
				}
				if (auto pT = registry.component<CData<Transform>>(eid0))
				{
					pT->data.setOrientation(glm::rotate(pT->data.orientation(), glm::radians(dt.to_s() * 12), glm::vec3(1.0f, 1.0f, 1.0f)));
				}
				transform1.setOrientation(glm::rotate(transform1.orientation(), glm::radians(dt.to_s() * 15), glm::vec3(0.0f, 1.0f, 0.0f)));
				if (auto pT = registry.component<CData<Transform>>(eid2))
				{
					pT->data.setOrientation(glm::rotate(pT->data.orientation(), glm::radians(dt.to_s() * 18), glm::vec3(0.3f, 1.0f, 1.0f)));
				}
				view0.mat_v = freeCam0.view();
				view0.pos_v = freeCam0.m_position;
				view1.mat_v = freeCam1.view();
				view1.pos_v = freeCam1.m_position;
				if (w0.isOpen())
				{
					auto const size = w0.framebufferSize();
					if (size.x > 0 && size.y > 0)
					{
						view0.mat_ui = freeCam0.ui({size, 2.0f});
						view0.mat_p = freeCam0.perspective((f32)size.x / (f32)size.y);
						view0.mat_vp = view0.mat_p * view0.mat_v;
					}
				}
				if (w1.isOpen())
				{
					auto const size = w1.framebufferSize();
					if (size.x > 0 && size.y > 0)
					{
						view1.mat_ui = freeCam1.ui({size, 2.0f});
						view1.mat_p = freeCam1.perspective((f32)size.x / (f32)size.y, 0.1f, 10.0f);
						view1.mat_vp = view1.mat_p * view1.mat_v;
					}
				}
			}

			if (w0.isClosing())
			{
				freeCam0.reset(false, false);
				w0.destroy();
			}
			if (w1.isClosing())
			{
				freeCam1.reset(false, false);
				w1.destroy();
			}
			if (bRecreate0)
			{
				bRecreate0 = false;
				w0.create(info0);
				pPipeline0 = createPipeline(&w0.renderer(), "default");
				pPipeline0wf = createPipeline(&w0.renderer(), "wireframe", gfx::PolygonMode::eLine);
				pSkyPipe = w0.renderer().createPipeline(skyboxPipeInfo);
			}
			if (bRecreate1)
			{
				bRecreate1 = false;
				w1.create(info1);
				pPipeline1 = createPipeline(&w1.renderer(), "default");
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
			ft = Time::elapsed() - fStart;
			// Render
#if defined(LEVK_DEBUG)
			try
#endif
			{
				if (w0.isOpen())
				{
					gfx::Renderer::Scene scene;
					scene.dirLights = {dirLight0, dirLight1};
					scene.clear.colour = Colour(0x030203ff);
					scene.view = view0;
					scene.view.skybox = {pCubemap, pSkyPipe};
					auto pPipeline = bWF0 ? pPipeline0wf : pPipeline0;
					gfx::Renderer::Batch batch;
					if (bTEMP)
					{
						batch.drawables.push_back({{pTriangle0}, &transform1, pPipeline});
					}
					{
						auto view = registry.view<CData<Transform>, CResource<gfx::Model>>();
						for (auto& [eid, query] : view)
						{
							if (auto pModel = query.get<CResource<gfx::Model>>()->get())
							{
								batch.drawables.push_back({pModel->meshes(), &query.get<CData<Transform>>()->data, pPipeline});
							}
						}
					}
					{
						auto view = registry.view<CData<Transform>, CResource<gfx::Mesh>>();
						for (auto& [eid, query] : view)
						{
							if (auto pMesh = query.get<CResource<gfx::Mesh>>()->get())
							{
								batch.drawables.push_back({{pMesh}, &query.get<CData<Transform>>()->data, pPipeline});
							}
						}
					}
					batch.drawables.push_back({{fpsText.mesh()}, &Transform::s_identity, pPipeline});
					batch.drawables.push_back({{ftText.mesh()}, &Transform::s_identity, pPipeline});
					scene.batches.push_back(std::move(batch));
					w0.renderer().render(std::move(scene));
				}
				if (w1.isOpen())
				{
					gfx::Renderer::Scene scene;
					scene.dirLights = {dirLight1};
					scene.clear.colour = Colour(0x030203ff);
					scene.view = view1;
					gfx::Renderer::Batch batch;
					if (pTriangle0->isReady())
					{
						batch.drawables = {{{pTriangle0}, &transform1, pPipeline1}};
					}
					{
						auto view = registry.view<CData<Transform>, CResource<gfx::Model>>();
						for (auto& [eid, query] : view)
						{
							if (auto pModel = query.get<CResource<gfx::Model>>()->get())
							{
								batch.drawables.push_back({pModel->meshes(), &query.get<CData<Transform>>()->data, pPipeline1});
							}
						}
					}
					{
						auto view = registry.view<CData<Transform>, CResource<gfx::Mesh>>();
						for (auto& [eid, query] : view)
						{
							if (auto pMesh = query.get<CResource<gfx::Mesh>>()->get())
							{
								batch.drawables.push_back({{pMesh}, &query.get<CData<Transform>>()->data, pPipeline1});
							}
						}
					}
					scene.batches.push_back(std::move(batch));
					w1.renderer().render(std::move(scene));
				}
			}
#if defined(LEVK_DEBUG)
			catch (std::exception const& e)
			{
				LOG_E("EXCEPTION!\n\t{}", e.what());
			}
#endif
		}
	}
	return 0;
}
