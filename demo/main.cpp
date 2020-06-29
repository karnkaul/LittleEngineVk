#include <initializer_list>
#include <core/assert.hpp>
#include <core/gdata.hpp>
#include <core/io.hpp>
#include <core/jobs.hpp>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <core/map_store.hpp>
#include <core/threads.hpp>
#include <core/transform.hpp>
#include <engine/levk.hpp>
#include <engine/assets/resources.hpp>
#include <engine/ecs/registry.hpp>
#include <engine/game/freecam.hpp>
#include <engine/game/input.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/game/world.hpp>
#include <engine/gfx/font.hpp>
#include <engine/gfx/geometry.hpp>
#include <engine/gfx/light.hpp>
#include <engine/gfx/mesh.hpp>
#include <engine/gfx/model.hpp>
#include <engine/gfx/renderer.hpp>
#include <engine/gfx/shader.hpp>
#include <engine/gfx/texture.hpp>
#include <engine/window/window.hpp>

using namespace le;

namespace
{
std::unique_ptr<IOReader> g_uReader;

class TestWorld : public World
{
private:
	struct
	{
		World::ID prevID;
		Entity mainText;
		Entity elapsedText;
		Time elapsed;
		bool bLoaded = false;
	} m_data;

	gfx::Camera m_camera;

protected:
	bool start() override
	{
		m_inputContext.context.mapTrigger("load_prev", [this]() {
			if (!loadWorld(m_previousWorldID))
			{
				LOG_E("[{}] Failed to load World{}", m_tName, m_previousWorldID);
			}
		});
		m_inputContext.context.addTrigger("load_prev", input::Key::eP, input::Action::eRelease, input::Mods::eCONTROL);
		m_data.mainText = m_registry.spawnEntity<UIComponent>("mainText");
		m_data.elapsedText = m_registry.spawnEntity<UIComponent>("elapsedText");
		gfx::Text2D::Info info;
		info.data.colour = colours::white;
		info.data.text = "Test World";
		info.data.scale = 0.25f;
		info.id = "title";
		m_registry.component<UIComponent>(m_data.mainText)->setText(std::move(info));
		info.data.text = "0";
		info.data.pos = {0.0f, -100.0f, 0.0f};
		info.id = "elapsed";
		m_data.elapsed = {};
		m_registry.component<UIComponent>(m_data.elapsedText)->setText(std::move(info));
		m_camera.m_position = {0.0f, 1.0f, 2.0f};
		return true;
	}

	void tick(Time dt) override
	{
		m_data.elapsed += dt;
		m_registry.component<UIComponent>(m_data.elapsedText)->uText->updateText(fmt::format("{:.1f}", m_data.elapsed.to_s()));
	}

	void stop() override
	{
		m_data = {};
	}

	gfx::Renderer::Scene buildScene() const override
	{
		return SceneBuilder(m_camera).build(m_registry);
	}

	stdfs::path manifestID() const override
	{
		return "test.manifest";
	}

	void onManifestLoaded() override
	{
		m_data.bLoaded = true;
	}
};

class DemoWorld : public World
{
public:
	u32 m_fps;

private:
	struct
	{
		stdfs::path model0id, model1id, skyboxID;
		std::vector<std::shared_ptr<HJob>> modelReloads;
		FreeCam freeCam;
		gfx::DirLight dirLight0, dirLight1;
		Entity eid0, eid1, eid2, eid3;
		Entity eui0, eui1, eui2;
		Entity skybox;
		Entity pointer;
		input::CtxWrapper temp;
		Time reloadTime;
		bool bLoadUnloadModels = false;
		bool bWireframe = false;
		bool bQuit = false;
	} m_data;
	struct
	{
		gfx::Mesh* pTriangle0 = nullptr;
		gfx::Mesh* pQuad = nullptr;
		gfx::Mesh* pSphere = nullptr;
	} m_res;
	gfx::Pipeline* m_pPipeline0wf = nullptr;

protected:
	bool start() override;
	void tick(Time dt) override;
	gfx::Renderer::Scene buildScene() const override;
	void stop() override;

	stdfs::path manifestID() const override;
	stdfs::path inputMapID() const override;
	void onManifestLoaded() override;
};

bool DemoWorld::start()
{
	gfx::Mesh::Info meshInfo;
	// clang-format off
	meshInfo.geometry.vertices = {
		{{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {}, {0.5f, 0.0f}},
		{{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {0.0f, 1.0f}},
	};
	// clang-format on
	m_res.pTriangle0 = Resources::inst().create<gfx::Mesh>("demo/triangle", meshInfo);
	meshInfo.geometry = gfx::createQuad();
	m_res.pQuad = Resources::inst().create<gfx::Mesh>("demo/quad", meshInfo);
	meshInfo.geometry = gfx::createCubedSphere(1.0f, 8);
	m_res.pSphere = Resources::inst().create<gfx::Mesh>("demo/sphere", meshInfo);

	m_data.skyboxID = "skyboxes/sky_dusk";
	m_data.model0id = "models/plant";
	m_data.model1id = g_uReader->isPresent("models/test/nanosuit/nanosuit.json") ? "models/test/nanosuit" : m_data.model0id;
	gfx::Material::Info texturedInfo;
	texturedInfo.albedo.ambient = Colour(0x888888ff);
	auto pTexturedLit = Resources::inst().create<gfx::Material>("materials/textured", texturedInfo);

	gfx::Texture::Info textureInfo;
	textureInfo.pReader = g_uReader.get();
	m_res.pQuad->m_material.flags.set({gfx::Material::Flag::eTextured, gfx::Material::Flag::eLit, gfx::Material::Flag::eOpaque});
	m_res.pQuad->m_material.pMaterial = pTexturedLit;
	m_res.pSphere->m_material.flags.set({gfx::Material::Flag::eTextured, gfx::Material::Flag::eLit, gfx::Material::Flag::eOpaque});
	m_res.pSphere->m_material.pMaterial = pTexturedLit;
	m_res.pSphere->m_material.tint.a = 0xcc;
	m_res.pQuad->m_material.pSpecular = nullptr;
	m_res.pQuad->m_material.flags.reset(gfx::Material::Flag::eOpaque);

	m_data.dirLight0.diffuse = Colour(0xffffffff);
	m_data.dirLight0.direction = glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f));
	m_data.dirLight1.diffuse = Colour(0xffffffff);
	m_data.dirLight1.direction = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

	m_data.eid0 = spawnEntity("quad");
	m_data.eid1 = spawnEntity("sphere");
	m_data.eid2 = spawnEntity("model0");
	m_data.eid3 = spawnEntity("model1");
	m_data.eui0 = spawnEntity("fps");
	m_data.eui1 = spawnEntity("dt", false);
	m_data.eui2 = spawnEntity("tris", false);
	m_data.skybox = spawnEntity("skybox", false);
	m_data.pointer = spawnEntity("pointer");

	gfx::Text2D::Info textInfo;
	textInfo.data.colour = colours::white;
	textInfo.data.scale = 0.25f;
	textInfo.id = "fps";
	m_registry.addComponent<UIComponent>(m_data.eui0)->setText(textInfo);
	textInfo.data.scale = 0.15f;
	textInfo.data.colour = colours::cyan;
	textInfo.data.pos.y -= 100.0f;
	textInfo.id = "ft";
	m_registry.addComponent<UIComponent>(m_data.eui1)->setText(textInfo);
	textInfo.data.colour = colours::yellow;
	textInfo.data.pos.y -= 100.0f;
	textInfo.data.pos.x = 620.0f;
	textInfo.id = "tris";
	m_registry.addComponent<UIComponent>(m_data.eui2)->setText(textInfo);
	m_registry.addComponent<UIComponent>(m_data.pointer)->setQuad({50.0f, 30.0f}, {25.0f, 15.0f}).m_material.tint = colours::cyan;

	m_registry.addComponent<TAsset<gfx::Texture>>(m_data.skybox)->id = m_data.skyboxID;

	m_data.freeCam.init();
	m_data.freeCam.m_position = {0.0f, 1.0f, 2.0f};
	m_data.freeCam.m_state.flags.set(FreeCam::Flag::eKeyToggle_Look);

	m_registry.component<Transform>(m_data.eid0)->setPosition({1.0f, 1.0f, -2.0f});
	m_registry.addComponent<TAsset<gfx::Mesh>>(m_data.eid0, m_res.pQuad->m_id);

	auto& t2 = m_registry.component<Transform>(m_data.eid1)->setPosition({0.0f, 0.0f, -2.0f});
	m_registry.addComponent<TAsset<gfx::Mesh>>(m_data.eid1, m_res.pSphere->m_id);

	m_registry.component<Transform>(m_data.eid2)->setPosition({-1.0f, 1.0f, -2.0f}).setParent(&t2);
	m_registry.addComponent<TAsset<gfx::Model>>(m_data.eid2, m_data.model0id);

	m_registry.component<Transform>(m_data.eid3)->setPosition({0.0f, -1.0f, -3.0f});
	m_registry.addComponent<TAsset<gfx::Model>>(m_data.eid3, m_data.model1id);

	if (!m_pPipeline0wf)
	{
		gfx::Pipeline::Info pipelineInfo;
		pipelineInfo.name = "wireframe";
		pipelineInfo.polygonMode = gfx::PolygonMode::eLine;
		m_pPipeline0wf = engine::mainWindow()->renderer().createPipeline(std::move(pipelineInfo));
	}

	m_inputContext.context.mapTrigger("wireframe", [this]() { m_data.bWireframe = !m_data.bWireframe; });
	m_inputContext.context.mapTrigger("reload_models", [this]() { m_data.bLoadUnloadModels = true; });
	m_inputContext.context.mapTrigger("quit", [this]() { m_data.bQuit = true; });
	m_inputContext.context.mapState("run", [](bool bActive) { LOGIF_I(bActive, "RUNNING!"); });
	m_inputContext.context.mapState("pause_cam", [this](bool bActive) { m_data.freeCam.m_state.flags[FreeCam::Flag::eEnabled] = !bActive; });
	m_inputContext.context.addState("pause_cam", input::Key::eLeftControl);
	m_inputContext.context.addState("pause_cam", input::Key::eRightControl);

#if defined(LEVK_DEBUG)
	m_data.temp.context.m_name = "Demo-Temp";
#endif
	m_data.temp.context.setMode(input::Mode::eBlockAll);
	m_data.temp.context.mapTrigger("test2", []() { LOG_I("Test2 triggered!"); });
	m_data.temp.context.addTrigger("test2", input::Key::eK);
	return true;
}

void DemoWorld::tick(Time dt)
{
	if (m_data.bQuit)
	{
		engine::mainWindow()->close();
		return;
	}

	static Time elapsed;
	elapsed += dt;
	if (elapsed >= 5s && !m_data.temp.token)
	{
		input::registerContext(m_data.temp);
	}
	if (elapsed >= 10s && m_data.temp.token)
	{
		m_data.temp.token.reset();
	}

	auto iter = std::remove_if(m_data.modelReloads.begin(), m_data.modelReloads.end(), [](auto sJob) -> bool { return sJob->hasCompleted(); });
	m_data.modelReloads.erase(iter, m_data.modelReloads.end());

	if (m_data.bLoadUnloadModels && m_data.modelReloads.empty())
	{
		if (Resources::inst().get<gfx::Model>(m_data.model0id))
		{
			Resources::inst().unload<gfx::Model>(m_data.model0id);
			if (Resources::inst().get<gfx::Model>(m_data.model1id))
			{
				Resources::inst().unload<gfx::Model>(m_data.model1id);
			}
		}
		else
		{
			m_data.modelReloads.push_back(jobs::enqueue(
				[this]() {
					auto semaphore = Resources::inst().setBusy();
					gfx::Model::LoadRequest mlr;
					mlr.assetID = m_data.model0id;
					mlr.pReader = g_uReader.get();
					auto m0info = gfx::Model::parseOBJ(mlr);
					Resources::inst().create<gfx::Model>(m_data.model0id, std::move(m0info));
				},
				"Model0-Reload"));
			if (m_data.model0id != m_data.model1id)
			{
				m_data.modelReloads.push_back(jobs::enqueue(
					[this]() {
						auto semaphore = Resources::inst().setBusy();
						gfx::Model::LoadRequest mlr;
						mlr.assetID = m_data.model1id;
						mlr.pReader = g_uReader.get();
						auto m1info = gfx::Model::parseOBJ(mlr);
						Resources::inst().create<gfx::Model>(m_data.model1id, std::move(m1info));
					},
					"Model1-Reload"));
			}
		}
		m_data.bLoadUnloadModels = false;
	}

	m_data.freeCam.tick(dt);

	m_registry.component<UIComponent>(m_data.eui0)->uText->updateText(fmt::format("{}FPS", m_fps));
	m_registry.component<UIComponent>(m_data.eui1)->uText->updateText(fmt::format("{:.3}ms", dt.to_s() * 1000));
	m_registry.component<UIComponent>(m_data.eui2)->uText->updateText(fmt::format("{} triangles", engine::mainWindow()->renderer().m_stats.trisDrawn));
	[[maybe_unused]] auto& quadT = m_registry.component<Transform>(m_data.pointer)->setPosition({input::worldToUI(input::cursorPosition()), 1.0f});
	// quadT.setOrientation(glm::rotate(quadT.orientation(), glm::radians(dt.to_s() * 50), gfx::g_nFront));

	{
		// Update matrices
		if (auto pT = m_registry.component<Transform>(m_data.eid1))
		{
			pT->setOrientation(glm::rotate(pT->orientation(), glm::radians(dt.to_s() * 10), glm::vec3(0.0f, 1.0f, 0.0f)));
		}
		if (auto pT = m_registry.component<Transform>(m_data.eid0))
		{
			pT->setOrientation(glm::rotate(pT->orientation(), glm::radians(dt.to_s() * 12), glm::vec3(1.0f, 1.0f, 1.0f)));
		}
		if (auto pT = m_registry.component<Transform>(m_data.eid2))
		{
			pT->setOrientation(glm::rotate(pT->orientation(), glm::radians(dt.to_s() * 18), glm::vec3(0.3f, 1.0f, 1.0f)));
		}
	}
}

gfx::Renderer::Scene DemoWorld::buildScene() const
{
	SceneBuilder::Info info;
	info.pCamera = &m_data.freeCam;
	info.dirLights = {m_data.dirLight0, m_data.dirLight1};
	info.clearColour = Colour(0x030203ff);
	info.p3Dpipe = m_data.bWireframe ? m_pPipeline0wf : nullptr;
	info.skyboxCubemapID = "skyboxes/sky_dusk";
	info.uiSpace = {1280.0f, 720.0f, 2.0f};
	info.flags = SceneBuilder::Flag::eScissoredUI;
	return SceneBuilder(std::move(info)).build(m_registry);
}

void DemoWorld::stop()
{
	auto unload = [](std::initializer_list<stdfs::path const*> ids) {
		for (auto pID : ids)
		{
			Resources::inst().unload(*pID);
		}
	};
	stdfs::path const matID = "materials/textured";
	unload({&m_res.pQuad->m_id, &m_res.pSphere->m_id, &m_res.pTriangle0->m_id, &matID});
	m_data = {};
	m_res = {};
}

stdfs::path DemoWorld::manifestID() const
{
	return "demo.manifest";
}

stdfs::path DemoWorld::inputMapID() const
{
	return "demo.input";
}

void DemoWorld::onManifestLoaded()
{
	m_res.pSphere->m_material.pDiffuse = Resources::inst().get<gfx::Texture>("textures/container2.png");
	m_res.pSphere->m_material.pSpecular = Resources::inst().get<gfx::Texture>("textures/container2_specular.png");
	m_res.pQuad->m_material.pDiffuse = Resources::inst().get<gfx::Texture>("textures/awesomeface.png");
}
} // namespace

int main(int argc, char** argv)
{
	engine::Service engine(argc, argv);
	g_uReader = std::make_unique<FileReader>();
	engine::Info info;
	Window::Info info0;
	info0.config.size = {1280, 720};
	info0.config.title = "LittleEngineVk Demo";
	info.windowInfo = info0;
	info.pReader = g_uReader.get();
	info.dataPaths = engine.locateData({{{"data"}}, {{"demo/data"}}});
	if (!engine.init(info))
	{
		return 1;
	}
	World::addWorld<DemoWorld, TestWorld>();
	auto pWorld = World::getWorld<DemoWorld>();

	if (!engine.start(0))
	{
		return 1;
	}

	Time t = Time::elapsed();
	while (Window::anyActive())
	{
		Time dt = Time::elapsed() - t;
		t = Time::elapsed();

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

			// Tick
			engine.tick(dt);

			pWorld->m_fps = fps == 0 ? frames : fps;
		}

		// Render
#if defined(LEVK_DEBUG)
		try
#endif
		{
			engine.submitScene();
			Window::renderAll();
		}
#if defined(LEVK_DEBUG)
		catch (std::exception const& e)
		{
			LOG_E("EXCEPTION!\n\t{}", e.what());
		}
#endif
	}
	return 0;
}
