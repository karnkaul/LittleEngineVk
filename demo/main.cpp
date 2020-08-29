#include <engine/levk.hpp>
#include <level.hpp>

#pragma region legacy
// namespace
// {
// class TestWorld : public legacy::World
// {
// private:
// 	struct
// 	{
// 		World::ID prevID;
// 		Entity mainText;
// 		Entity elapsedText;
// 		Time elapsed;
// 		bool bLoaded = false;
// 	} m_data;

// protected:
// 	bool start() override
// 	{
// 		m_inputContext.mapTrigger("load_prev", [this]() {
// 			if (!loadWorld(m_previousWorldID))
// 			{
// 				LOG_E("[{}] Failed to load World{}", m_name, m_previousWorldID);
// 			}
// 		});
// 		m_inputContext.addTrigger("load_prev", input::Key::eP, input::Action::eRelease, input::Mods::eCONTROL);
// 		m_data.mainText = m_registry.spawnEntity<UIComponent>("mainText");
// 		m_data.elapsedText = m_registry.spawnEntity<UIComponent>("elapsedText");
// 		Text2D::Info info;
// 		info.data.colour = colours::white;
// 		info.data.text = "Test World";
// 		info.data.scale = 0.25f;
// 		info.id = "title";
// 		m_registry.component<UIComponent>(m_data.mainText)->setText(std::move(info));
// 		info.data.text = "0";
// 		info.data.pos = {0.0f, -100.0f, 0.0f};
// 		info.id = "elapsed";
// 		m_data.elapsed = {};
// 		m_registry.component<UIComponent>(m_data.elapsedText)->setText(std::move(info));
// 		m_defaultCam.reset();
// 		m_defaultCam.position = {0.0f, 1.0f, 2.0f};
// 		return true;
// 	}

// 	void tick(Time dt) override
// 	{
// 		m_data.elapsed += dt;
// 		m_registry.component<UIComponent>(m_data.elapsedText)->setText(fmt::format("{:.1f}", m_data.elapsed.to_s()));
// 	}

// 	void stop() override
// 	{
// 		m_data = {};
// 	}

// 	stdfs::path manifestID() const override
// 	{
// 		return "test.manifest";
// 	}

// 	void onManifestLoaded() override
// 	{
// 		m_data.bLoaded = true;
// 	}
// };

// class DemoWorld : public legacy::World
// {
// public:
// 	u32 m_fps;

// private:
// 	struct
// 	{
// 		stdfs::path model0id, model1id, skyboxID;
// 		res::Async<res::Model> asyncModel0, asyncModel1;
// 		gfx::DirLight dirLight0, dirLight1;
// 		Entity eid0, eid1, eid2, eid3;
// 		Entity eui0, eui1, eui2;
// 		Entity skybox;
// 		Entity pointer;
// 		input::Context temp;
// 		FreeCam freeCam;
// 		Token tempToken;
// 		Time reloadTime;
// 		bool bLoadUnloadModels = false;
// 		bool bWireframe = false;
// 		bool bQuit = false;
// 	} m_data;
// 	struct
// 	{
// 		res::Scoped<res::Material> texturedLit;
// 		res::Scoped<res::Mesh> triangle;
// 		res::Scoped<res::Mesh> quad;
// 		res::Scoped<res::Mesh> sphere;
// 		Hash container2 = "textures/container2.png";
// 		Hash container2_specular = "textures/container2_specular.png";
// 		Hash awesomeface = "textures/awesomeface.png";
// 	} m_res;
// 	gfx::Pipeline* m_pPipeline0wf = nullptr;

// protected:
// 	bool start() override;
// 	void tick(Time dt) override;
// 	SceneBuilder const& sceneBuilder() const override;
// 	void stop() override;

// 	stdfs::path manifestID() const override;
// 	stdfs::path inputMapID() const override;
// 	void onManifestLoaded() override;

// 	gfx::Camera const& camera() const override;
// };

// bool DemoWorld::start()
// {
// 	res::Material::CreateInfo texturedInfo;
// 	texturedInfo.albedo.ambient = Colour(0x888888ff);
// 	m_res.texturedLit = res::load("materials/textured", texturedInfo);

// 	res::Mesh::CreateInfo meshInfo;
// 	// clang-format off
// 	meshInfo.geometry.vertices = {
// 		{{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {}, {0.5f, 0.0f}},
// 		{{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {}, {1.0f, 1.0f}},
// 		{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {0.0f, 1.0f}},
// 	};
// 	// clang-format on
// 	m_res.triangle = res::load("demo/triangle", meshInfo);
// 	meshInfo.geometry = gfx::createQuad();
// 	meshInfo.material.flags.set({res::Material::Flag::eTextured, res::Material::Flag::eLit, res::Material::Flag::eOpaque});
// 	meshInfo.material.material = m_res.texturedLit;
// 	meshInfo.material.flags.reset(res::Material::Flag::eOpaque);
// 	m_res.quad = res::load("demo/quad", meshInfo);
// 	meshInfo.geometry = gfx::createCubedSphere(1.0f, 8);
// 	meshInfo.material.tint.a = 0xcc;
// 	meshInfo.material.flags.set(res::Material::Flag::eOpaque);
// 	m_res.sphere = res::load("demo/sphere", meshInfo);

// 	m_data.skyboxID = "skyboxes/sky_dusk";
// 	m_data.model0id = "models/plant";
// 	m_data.model1id = engine::reader().isPresent("models/test/nanosuit/nanosuit.json") ? "models/test/nanosuit" : m_data.model0id;

// 	m_data.dirLight0.diffuse = Colour(0xffffffff);
// 	m_data.dirLight0.direction = glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f));
// 	m_data.dirLight1.diffuse = Colour(0xffffffff);
// 	m_data.dirLight1.direction = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

// 	m_data.eid0 = spawnEntity("quad");
// 	m_data.eid1 = spawnEntity("sphere");
// 	m_data.eid2 = spawnEntity("model0");
// 	m_data.eid3 = spawnEntity("model1");
// 	m_data.eui0 = spawnEntity("fps");
// 	m_data.eui1 = spawnEntity("dt", false);
// 	m_data.eui2 = spawnEntity("tris", false);
// 	m_data.skybox = spawnEntity("skybox", false);
// 	m_data.pointer = spawnEntity("pointer");

// 	Text2D::Info textInfo;
// 	textInfo.data.colour = colours::white;
// 	textInfo.data.scale = 0.25f;
// 	textInfo.id = "fps";
// 	m_registry.addComponent<UIComponent>(m_data.eui0)->setText(textInfo);
// 	textInfo.data.scale = 0.15f;
// 	textInfo.data.colour = colours::cyan;
// 	textInfo.data.pos.y -= 100.0f;
// 	textInfo.id = "ft";
// 	m_registry.addComponent<UIComponent>(m_data.eui1)->setText(textInfo);
// 	textInfo.data.colour = colours::yellow;
// 	textInfo.data.pos.y -= 100.0f;
// 	textInfo.data.pos.x = 620.0f;
// 	textInfo.id = "tris";
// 	m_registry.addComponent<UIComponent>(m_data.eui2)->setText(textInfo);
// 	m_registry.addComponent<UIComponent>(m_data.pointer)->setQuad({50.0f, 30.0f}, {25.0f, 15.0f}).material().tint = colours::cyan;

// 	m_data.freeCam.init();
// 	m_data.freeCam.m_state.flags.set(FreeCam::Flag::eKeyToggle_Look);
// 	m_data.freeCam.m_camera.position = {0.0f, 1.0f, 2.0f};

// 	m_registry.component<Transform>(m_data.eid0)->position({1.0f, 1.0f, -2.0f});
// 	m_registry.addComponent<res::Mesh>(m_data.eid0, m_res.quad);

// 	auto& t2 = m_registry.component<Transform>(m_data.eid1)->position({0.0f, 0.0f, -2.0f});
// 	m_registry.addComponent<res::Mesh>(m_data.eid1, m_res.sphere);

// 	m_registry.component<Transform>(m_data.eid2)->position({-1.0f, 1.0f, -2.0f}).parent(&t2);
// 	m_registry.addComponent<res::Model>(m_data.eid2);

// 	m_registry.component<Transform>(m_data.eid3)->position({0.0f, -1.0f, -3.0f});
// 	m_registry.addComponent<res::Model>(m_data.eid3);

// 	if (!m_pPipeline0wf)
// 	{
// 		gfx::Pipeline::Info pipelineInfo;
// 		pipelineInfo.name = "wireframe";
// 		pipelineInfo.polygonMode = gfx::PolygonMode::eLine;
// 		m_pPipeline0wf = engine::mainWindow()->renderer().createPipeline(std::move(pipelineInfo));
// 	}

// 	m_inputContext.mapTrigger("wireframe", [this]() { m_data.bWireframe = !m_data.bWireframe; });
// 	m_inputContext.mapTrigger("reload_models", [this]() { m_data.bLoadUnloadModels = true; });
// 	m_inputContext.mapTrigger("quit", [this]() { m_data.bQuit = true; });
// 	m_inputContext.mapState("run", [](bool bActive) { LOGIF_I(bActive, "RUNNING!"); });
// 	m_inputContext.mapState("pause_cam", [this](bool bActive) { m_data.freeCam.m_state.flags[FreeCam::Flag::eEnabled] = !bActive; });
// 	m_inputContext.addState("pause_cam", input::Key::eLeftControl);
// 	m_inputContext.addState("pause_cam", input::Key::eRightControl);

// #if defined(LEVK_DEBUG)
// 	m_data.temp = {};
// 	m_data.temp.m_name = "Demo-Temp";
// #endif
// 	m_data.temp.setMode(input::Mode::eBlockAll);
// 	m_data.temp.mapTrigger("test2", []() { LOG_I("Test2 triggered!"); });
// 	m_data.temp.addTrigger("test2", input::Key::eK);
// 	return true;
// }

// void DemoWorld::tick(Time dt)
// {
// 	if (m_data.bQuit)
// 	{
// 		engine::Service::shutdown();
// 		return;
// 	}

// 	static Time elapsed;
// 	elapsed += dt;
// 	static bool s_bRegistered = false;
// 	if (elapsed >= 5s && !s_bRegistered)
// 	{
// 		m_data.tempToken = input::registerContext(&m_data.temp);
// 		s_bRegistered = true;
// 	}
// 	if (elapsed >= 8s && m_data.tempToken.valid())
// 	{
// 		m_data.tempToken = {};
// 	}

// 	if (m_data.asyncModel0.loaded())
// 	{
// 		if (auto pModel = m_registry.component<res::Model>(m_data.eid2))
// 		{
// 			*pModel = m_data.asyncModel0.resource().payload;
// 		}
// 		m_data.asyncModel0 = {};
// 	}
// 	if (m_data.asyncModel1.loaded())
// 	{
// 		if (auto pModel = m_registry.component<res::Model>(m_data.eid3))
// 		{
// 			*pModel = m_data.asyncModel1.resource().payload;
// 		}
// 		m_data.asyncModel1 = {};
// 	}
// 	if (m_data.bLoadUnloadModels && !m_data.asyncModel0.valid())
// 	{
// 		if (auto model = res::find<res::Model>(m_data.model0id))
// 		{
// 			res::unload(*model);
// 			if (auto model1 = res::find<res::Model>(m_data.model1id))
// 			{
// 				res::unload(*model1);
// 			}
// 			m_data.asyncModel0 = m_data.asyncModel1 = {};
// 		}
// 		else
// 		{
// 			res::Model::LoadInfo loadInfo;
// 			loadInfo.idRoot = loadInfo.jsonDirectory = m_data.model0id;
// 			m_data.asyncModel0 = res::loadAsync(m_data.model0id, std::move(loadInfo));
// 			if (m_data.model1id != m_data.model0id)
// 			{
// 				loadInfo = {};
// 				loadInfo.idRoot = loadInfo.jsonDirectory = m_data.model1id;
// 				m_data.asyncModel1 = res::loadAsync(m_data.model1id, std::move(loadInfo));
// 			}
// 		}
// 		m_data.bLoadUnloadModels = false;
// 	}

// 	m_data.freeCam.tick(dt);

// 	m_registry.component<UIComponent>(m_data.eui0)->setText(fmt::format("{}FPS", m_fps));
// 	m_registry.component<UIComponent>(m_data.eui1)->setText(fmt::format("{:.3}ms", dt.to_s() * 1000));
// 	m_registry.component<UIComponent>(m_data.eui2)->setText(fmt::format("{} triangles", engine::mainWindow()->renderer().m_stats.trisDrawn));
// 	[[maybe_unused]] auto& quadT = m_registry.component<Transform>(m_data.pointer)->position({input::worldToUI(input::cursorPosition()), 1.0f});
// 	// quadT.orientglm::rotate(quadT.orient), glm::radians(dt.to_s() * 50), gfx::g_nFront));

// 	{
// 		// Update matrices
// 		if (auto pT = m_registry.component<Transform>(m_data.eid1))
// 		{
// 			pT->orient(glm::rotate(pT->orientation(), glm::radians(dt.to_s() * 10), glm::vec3(0.0f, 1.0f, 0.0f)));
// 		}
// 		if (auto pT = m_registry.component<Transform>(m_data.eid0))
// 		{
// 			pT->orient(glm::rotate(pT->orientation(), glm::radians(dt.to_s() * 12), glm::vec3(1.0f, 1.0f, 1.0f)));
// 		}
// 		if (auto pT = m_registry.component<Transform>(m_data.eid2))
// 		{
// 			pT->orient(glm::rotate(pT->orientation(), glm::radians(dt.to_s() * 18), glm::vec3(0.3f, 1.0f, 1.0f)));
// 		}
// 	}
// }

// SceneBuilder const& DemoWorld::sceneBuilder() const
// {
// 	static SceneBuilder s_sceneBuilder;
// 	SceneBuilder::Info info;
// 	info.dirLights = {m_data.dirLight0, m_data.dirLight1};
// 	info.clearColour = Colour(0x030203ff);
// 	info.p3Dpipe = m_data.bWireframe ? m_pPipeline0wf : nullptr;
// 	info.skyboxCubemapID = "skyboxes/sky_dusk";
// 	info.uiSpace = {1280.0f, 720.0f, 2.0f};
// 	info.flags = SceneBuilder::Flag::eScissoredUI;
// 	s_sceneBuilder = SceneBuilder(std::move(info));
// 	return s_sceneBuilder;
// }

// void DemoWorld::stop()
// {
// 	stdfs::path const matID = "materials/textured";
// 	m_data = {};
// 	m_res = {};
// }

// stdfs::path DemoWorld::manifestID() const
// {
// 	return "demo.manifest";
// }

// stdfs::path DemoWorld::inputMapID() const
// {
// 	return "demo.input";
// }

// void DemoWorld::onManifestLoaded()
// {
// 	m_res.sphere.resource.material().diffuse = res::find<res::Texture>(m_res.container2).payload;
// 	m_res.sphere.resource.material().specular = res::find<res::Texture>(m_res.container2_specular).payload;
// 	m_res.quad.resource.material().diffuse = res::find<res::Texture>(m_res.awesomeface).payload;
// 	if (auto pModel = m_registry.component<res::Model>(m_data.eid3))
// 	{
// 		*pModel = res::find<res::Model>(m_data.model1id).payload;
// 	}
// 	if (auto pModel = m_registry.component<res::Model>(m_data.eid2))
// 	{
// 		*pModel = res::find<res::Model>(m_data.model0id).payload;
// 	}
// }

// gfx::Camera const& DemoWorld::camera() const
// {
// 	return m_data.freeCam.m_camera;
// }
// } // namespace
#pragma endregion legacy

int main(int argc, char** argv)
{
	using namespace le;
	engine::Service engine(argc, argv);

	io::FileReader reader;
	engine::Info info;
	Window::Info info0;
	info0.config.size = {1280, 720};
	info0.config.title = "LittleEngineVk Demo";
	info.windowInfo = info0;
	info.pReader = &reader;
	info.dataPaths = engine.locateData({{{"data"}}, {{"demo/data"}}});
	// info.dataPaths = engine.locateData({{{"data.zip"}}, {{"demo/data.zip"}}});
#if defined(LEVK_DEBUG)
	info.bLogVRAMallocations = true;
#endif
	if (!engine.init(info))
	{
		return 1;
	}
	engine::g_shutdownSequence = engine::ShutdownSequence::eShutdown_CloseWindow;
	while (engine.running())
	{
		engine.update(g_driver);
		engine.render();
	}
	g_driver.cleanup();
	return 0;
}
