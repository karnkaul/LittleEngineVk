#include <imgui.h>
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
#include "engine/ecs/registry.hpp"
#include "engine/editor/editor.hpp"
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

using namespace le;

namespace
{
std::unique_ptr<IOReader> g_uReader;
} // namespace

int main(int argc, char** argv)
{
	engine::Service engine(argc, argv);
	g_uReader = std::make_unique<FileReader>();
	{
		auto [engineData, bResult] = FileReader::findUpwards(engine::exePath(), {"engine_data"});
		if (!bResult)
		{
			LOG_E("Failed to locate engine data!");
			return 1;
		}
		if (!g_uReader->mount(engineData))
		{
			LOG_E("Failed to mount engine data [{}]!", engineData.generic_string());
			return 1;
		}
	}
	{
		auto [dataPath, bResult] = FileReader::findUpwards(engine::exePath(), {"demo/data"});
		if (!bResult)
		{
			LOG_E("FATAL: Could not locate demo data!");
			return 1;
		}
		if (!g_uReader->mount(dataPath))
		{
			return 1;
		}
	}
	if (!engine.start(*g_uReader))
	{
		return 1;
	}
	Registry registry;
	gfx::Mesh::Info triangle0info;
	// clang-format off
		triangle0info.geometry.vertices = {
			{{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {}, {0.5f, 0.0f}},
			{{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {}, {1.0f, 1.0f}},
			{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {0.0f, 1.0f}},
		};
	// clang-format on
	gfx::Mesh* pTriangle0 = Resources::inst().create<gfx::Mesh>("meshes/triangle0", triangle0info);

	gfx::Mesh::Info meshInfo;
	meshInfo.geometry = gfx::createQuad();
	gfx::Mesh* pMesh0 = Resources::inst().create<gfx::Mesh>("mesh0", meshInfo);
	meshInfo.geometry = gfx::createCubedSphere(1.0f, 8);
	gfx::Mesh* pMesh1 = Resources::inst().create<gfx::Mesh>("mesh1", meshInfo);

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
		pModel0 = Resources::inst().create<gfx::Model>(model0id, model0info);
	});
	jobs::enqueue([&pModel1, &model0lr, &model1id]() {
		auto model1info = gfx::Model::parseOBJ(model0lr);
		model1id = model1info.id;
		model1id += "_1";
		model1info.mode = gfx::Texture::Space::eRGBLinear;
		pModel1 = Resources::inst().create<gfx::Model>(model1id, model1info);
	});
	gfx::Material::Info texturedInfo;
	texturedInfo.albedo.ambient = Colour(0x888888ff);
	auto pTexturedLit = Resources::inst().create<gfx::Material>("materials/textured", texturedInfo);

	gfx::Texture::Info textureInfo;
	textureInfo.pReader = g_uReader.get();
	pMesh0->m_material.flags.set({gfx::Material::Flag::eTextured, gfx::Material::Flag::eLit, gfx::Material::Flag::eOpaque});
	pMesh0->m_material.pMaterial = pTexturedLit;
	pMesh1->m_material.flags.set({gfx::Material::Flag::eTextured, gfx::Material::Flag::eLit, gfx::Material::Flag::eOpaque});
	pMesh1->m_material.pMaterial = pTexturedLit;
	pMesh1->m_material.tint.a = 0xcc;
	textureInfo.ids = {"textures/container2.png"};
	pMesh1->m_material.pDiffuse = pMesh0->m_material.pDiffuse =
		Resources::inst().create<gfx::Texture>(textureInfo.ids.front(), textureInfo);
	textureInfo.ids = {"textures/container2_specular.png"};
	pMesh1->m_material.pSpecular = pMesh0->m_material.pSpecular =
		Resources::inst().create<gfx::Texture>(textureInfo.ids.front(), textureInfo);
	textureInfo.ids = {"textures/awesomeface.png"};
	pMesh0->m_material.pDiffuse = Resources::inst().create<gfx::Texture>(textureInfo.ids.front(), textureInfo);
	pMesh0->m_material.pSpecular = nullptr;
	pMesh0->m_material.flags.reset(gfx::Material::Flag::eOpaque);

	gfx::DirLight dirLight0;
	dirLight0.diffuse = Colour(0xffffffff);
	dirLight0.direction = glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f));
	gfx::DirLight dirLight1;
	dirLight1.diffuse = Colour(0xffffffff);
	dirLight1.direction = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

	gfx::Texture::Info cubemapInfo;
	cubemapInfo.pReader = g_uReader.get();
	cubemapInfo.type = gfx::Texture::Type::eCube;
	stdfs::path const& cp = "skyboxes/sky_dusk";
	cubemapInfo.ids = {cp / "right.jpg", cp / "left.jpg", cp / "up.jpg", cp / "down.jpg", cp / "front.jpg", cp / "back.jpg"};
	auto pCubemap = Resources::inst().create<gfx::Texture>("skyboxes/sky_dusk_cubemap", cubemapInfo);

	Window w0, w1;
	Window::Info info0;
	info0.config.size = {1280, 720};
	info0.config.title = "LittleEngineVk Demo";
	info0.config.centreOffset = {-200, -200};
	auto info1 = info0;
	info0.config.bEnableGUI = true;
	info1.options.colourSpaces.push_back({ColourSpace::eRGBLinear});
	// info1.config.mode = Window::Mode::eBorderlessFullscreen;
	info1.config.title += " 2";
	info1.config.centreOffset = {200, 200};
	bool bRecreate0 = false, bRecreate1 = false;
	bool bClose0 = false, bClose1 = false;
	bool bWF0 = false;
	bool bDisableCam = false;
	bool bTEMP = false;
	bool bToggleModel0 = false;
	bool bEditor = false;
	bool bAltPressed = false;
	gfx::Model::Info m0info;
	gfx::Model::Info m1info;
	Time reloadTime;
	auto onInput = [&w0, &w1, eid0, &registry, &bWF0, &bTEMP, &bToggleModel0, &bEditor, &bRecreate0, &bRecreate1, &bClose0, &bClose1,
					&bDisableCam, &bAltPressed](Key key, Action action, Mods mods) {
		if (key == Key::eW && action == Action::eRelease && mods & Mods::eCONTROL)
		{
			bClose0 = w0.isFocused();
			bClose1 = w1.isFocused();
		}
		if ((key == Key::eT || key == Key::eN) && action == Action::eRelease && mods & Mods::eCONTROL)
		{
			bRecreate0 = !w0.isOpen();
			bRecreate1 = !w1.isOpen();
		}
		if (key == Key::eLeftAlt || key == Key::eRightAlt)
		{
			if (action == Action::ePress)
			{
				bAltPressed = true;
			}
			else if (action == Action::eRelease)
			{
				bAltPressed = false;
			}
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
		if (key == Key::eF3 && action == Action::eRelease)
		{
			bEditor = !bEditor;
		}
		if (key == Key::eD && action == Action::eRelease && (mods & Mods::eCONTROL))
		{
			registry.destroyEntity(eid0);
		}
		if (key == Key::eE && action == Action::eRelease && (mods & Mods::eCONTROL))
		{
			registry.setEnabled(eid0, !registry.isEnabled(eid0));
		}
	};
	auto token = Window::registerInput(onInput, {});

	gfx::FreeCam freeCam0(&w0), freeCam1(&w1);
	freeCam0.m_state.flags.set(gfx::FreeCam::Flag::eKeyToggle_Look);
	freeCam1.m_state.flags = freeCam0.m_state.flags;

	gfx::Text2D fpsText, ftText, triText;
	gfx::Text2D::Info textInfo;
	textInfo.data.colour = colours::white;
	textInfo.data.scale = 0.25f;
	textInfo.id = "fps";
	fpsText.setup(textInfo);
	textInfo.data.scale = 0.15f;
	textInfo.data.colour = colours::cyan;
	textInfo.data.pos.y -= 100.0f;
	textInfo.id = "ft";
	ftText.setup(textInfo);
	textInfo.data.colour = colours::yellow;
	textInfo.data.pos.y -= 100.0f;
	textInfo.id = "tris";
	triText.setup(textInfo);

	gfx::ScreenRect gameRect;

	if (/*w1.create(info1) &&*/ w0.create(info0))
	{
		gfx::Pipeline* pPipeline0wf = nullptr;

		if (w0.isOpen())
		{
			gfx::Pipeline::Info pipelineInfo;
			pipelineInfo.name = "wireframe";
			pipelineInfo.polygonMode = gfx::PolygonMode::eLine;
			pPipeline0wf = w0.renderer().createPipeline(std::move(pipelineInfo));
		}

		gfx::Renderer::View view0;
		gfx::Renderer::View view1;
		freeCam0.m_position = {0.0f, 1.0f, 2.0f};

		registry.addComponent<Transform>(eid0)->setPosition({1.0f, 1.0f, -2.0f});
		registry.addComponent<TAsset<gfx::Mesh>>(eid0, pMesh0->m_id);

		registry.addComponent<Transform>(eid1)->setPosition({0.0f, 0.0f, -2.0f});
		registry.addComponent<TAsset<gfx::Mesh>>(eid1, pMesh1->m_id);

		registry.addComponent<Transform>(eid2)->setPosition({-1.0f, 1.0f, -2.0f});
		registry.addComponent<TAsset<gfx::Model>>(eid2, model0id);

		registry.addComponent<Transform>(eid3)->setPosition({0.0f, -1.0f, -3.0f});
		registry.addComponent<TAsset<gfx::Model>>(eid3, model1id);

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
			if (fpsElapsed >= 1s)
			{
				fps = frames;
				frames = 0;
				fpsElapsed = Time();
			}
			{
				// handle events
				Window::pollEvents();
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
					gfx::Pipeline::Info pipelineInfo;
					pipelineInfo.name = "wireframe";
					pipelineInfo.polygonMode = gfx::PolygonMode::eLine;
					pPipeline0wf = w0.renderer().createPipeline(std::move(pipelineInfo));
				}
				if (bRecreate1)
				{
					bRecreate1 = false;
					w1.create(info1);
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

				// Tick
				engine.update();
				registry.sweep();
				gameRect = {};

#if defined(LEVK_EDITOR)
				static auto const smol = glm::vec2(0.66f);
				if (bEditor && w0.isOpen())
				{
					static glm::vec2 centre = glm::vec2(0.5f * (f32)w0.windowSize().x, 0.0f);
					if (bAltPressed)
					{
						centre = w0.cursorPos();
					}
					gameRect = w0.renderer().clampToView(centre, smol);
					editor::render(gameRect, w0.framebufferSize());
				}
#endif
				if (bToggleModel0)
				{
					if (pModel0 && pModel1)
					{
						Resources::inst().unload<gfx::Model>(pModel0->m_id);
						Resources::inst().unload<gfx::Model>(pModel1->m_id);
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
								pModel0 = Resources::inst().create<gfx::Model>(model0id, std::move(m0info));
								LOG_I("{} total load time: {}s", m0info.id.generic_string(), (Time::elapsed() - reloadTime).to_s());
							},
							"Model0-Reload");
						jobs::enqueue(
							[&]() {
								m1info = gfx::Model::parseOBJ(model0lr);
								LOG_I("{} data loaded in: {}s", m1info.id.generic_string(), (Time::elapsed() - reloadTime).to_s());
								auto model1id = m1info.id;
								m1info.mode = gfx::Texture::Space::eRGBLinear;
								model1id += "_1";
								pModel1 = Resources::inst().create<gfx::Model>(model1id, std::move(m1info));
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
				ftText.updateText(fmt::format("{:.3}ms", dt.to_s() * 1000));
				triText.updateText(fmt::format("{} triangles", w0.renderer().m_stats.trisDrawn));

				if (auto pM = registry.component<TAsset<gfx::Model>>(eid2))
				{
					pM->id = model0id;
				}
				if (auto pM = registry.component<TAsset<gfx::Model>>(eid3))
				{
					pM->id = model1id;
				}

#if defined(LEVK_EDITOR)
				if (editor::g_bTickGame)
#endif
				{
					// Update matrices
					if (auto pT = registry.component<Transform>(eid1))
					{
						pT->setOrientation(glm::rotate(pT->orientation(), glm::radians(dt.to_s() * 10), glm::vec3(0.0f, 1.0f, 0.0f)));
					}
					if (auto pT = registry.component<Transform>(eid0))
					{
						pT->setOrientation(glm::rotate(pT->orientation(), glm::radians(dt.to_s() * 12), glm::vec3(1.0f, 1.0f, 1.0f)));
					}
					transform1.setOrientation(
						glm::rotate(transform1.orientation(), glm::radians(dt.to_s() * 15), glm::vec3(0.0f, 1.0f, 0.0f)));
					if (auto pT = registry.component<Transform>(eid2))
					{
						pT->setOrientation(glm::rotate(pT->orientation(), glm::radians(dt.to_s() * 18), glm::vec3(0.3f, 1.0f, 1.0f)));
					}
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

			// GUI
			static bool s_bImGuiDemo = false;
			if (s_bImGuiDemo)
			{
				GUI(ImGui::ShowDemoWindow(&s_bImGuiDemo));
			}

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
					scene.view.skybox.pCubemap = pCubemap;
					auto pPipeline = bWF0 ? pPipeline0wf : nullptr;
					gfx::Renderer::Batch batch;
					batch.viewport = scene.view.skybox.viewport = gameRect;
					if (bTEMP)
					{
						batch.drawables.push_back({{pTriangle0}, &transform1, pPipeline});
					}
					{
						auto const& r = registry;
						auto view = r.view<Transform, TAsset<gfx::Model>>();
						for (auto& query : view)
						{
							auto& [pTransform, cModel] = query;
							if (auto pModel = cModel->get(); pModel && pModel->isReady())
							{
								batch.drawables.push_back({pModel->meshes(), pTransform, pPipeline});
							}
						}
					}
					{
						auto view = registry.view<Transform, TAsset<gfx::Mesh>>();
						for (auto& query : view)
						{
							auto& [pTransform, cMesh] = query;
							if (auto pMesh = cMesh->get(); pMesh && pMesh->isReady())
							{
								batch.drawables.push_back({{pMesh}, pTransform, pPipeline});
							}
						}
					}
					batch.drawables.push_back({{fpsText.mesh()}, &Transform::s_identity, pPipeline});
					batch.drawables.push_back({{ftText.mesh()}, &Transform::s_identity, pPipeline});
					batch.drawables.push_back({{triText.mesh()}, &Transform::s_identity, pPipeline});
					scene.batches.push_back(std::move(batch));
					w0.renderer().submit(std::move(scene));
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
						batch.drawables = {{{pTriangle0}, &transform1}};
					}
					{
						auto view = registry.view<Transform, TAsset<gfx::Model>>();
						for (auto& query : view)
						{
							auto& [pTransform, cModel] = query;
							if (auto pModel = cModel->get(); pModel && pModel->isReady())
							{
								batch.drawables.push_back({pModel->meshes(), pTransform});
							}
						}
					}
					{
						auto view = registry.view<Transform, TAsset<gfx::Mesh>>();
						for (auto& query : view)
						{
							auto& [pTransform, cMesh] = query;
							if (auto pMesh = cMesh->get(); pMesh && pMesh->isReady())
							{
								batch.drawables.push_back({{pMesh}, pTransform});
							}
						}
					}
					scene.batches.push_back(std::move(batch));
					w1.renderer().submit(std::move(scene));
				}
				Window::renderAll();
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
