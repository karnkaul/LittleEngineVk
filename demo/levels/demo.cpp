#include <core/log.hpp>
#include <engine/levk.hpp>
#include <levels/demo.hpp>

using namespace le;

DemoLevel::DemoLevel()
{
	m_name = "Demo";
	m_manifest.id = "demo.manifest";
	m_input.id = "demo.input";

	res::Material::CreateInfo texturedInfo;
	texturedInfo.albedo.ambient = Colour(0x888888ff);
	m_res.texturedLit = res::load("materials/textured", texturedInfo);

	res::Mesh::CreateInfo meshInfo;
	// clang-format off
	meshInfo.geometry.vertices = {
		{{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {}, {0.5f, 0.0f}},
		{{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {}, {1.0f, 1.0f}},
		{{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {0.0f, 1.0f}},
	};
	// clang-format on
	m_res.triangle = res::load("demo/triangle", meshInfo);
	meshInfo.geometry = gfx::createQuad();
	meshInfo.material.flags.set(res::Material::Flag::eTextured | res::Material::Flag::eLit | res::Material::Flag::eOpaque);
	meshInfo.material.material = m_res.texturedLit;
	meshInfo.material.flags.reset(res::Material::Flag::eOpaque);
	m_res.quad = res::load("demo/quad", meshInfo);
	meshInfo.geometry = gfx::createCubedSphere(1.0f, 8);
	meshInfo.material.tint.a = 0xcc;
	meshInfo.material.flags.set(res::Material::Flag::eOpaque);
	m_res.sphere = res::load("demo/sphere", meshInfo);

	m_data.skyboxID = "skyboxes/sky_dusk";
	m_data.model0id = "models/plant";
	m_data.model1id = engine::reader().isPresent("models/test/nanosuit/nanosuit.json") ? "models/test/nanosuit" : m_data.model0id;

	m_data.dirLight0.diffuse = Colour(0xffffffff);
	m_data.dirLight0.direction = glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f));
	m_data.dirLight1.diffuse = Colour(0xffffffff);
	m_data.dirLight1.direction = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

	m_data.eid0 = gs::spawnProp("quad");
	m_data.eid1 = gs::spawnProp("sphere");
	m_data.eid2 = gs::spawnProp("model0");
	m_data.eid3 = gs::spawnProp("model1");
	auto [eui0, ui0c] = gs::spawnProp<UIComponent>("fps");
	m_data.eui0 = eui0;
	auto& [tui0] = ui0c;

	m_data.eui1 = registry().spawn("dt");
	m_data.eui2 = registry().spawn("tris");
	m_data.pointer = gs::spawnProp("pointer");

	Text2D::Info textInfo;
	textInfo.data.colour = colours::white;
	textInfo.data.scale = 0.25f;
	textInfo.id = "fps";
	tui0.setText(textInfo);
	textInfo.data.scale = 0.15f;
	textInfo.data.colour = colours::cyan;
	textInfo.data.pos.y -= 100.0f;
	textInfo.id = "ft";
	registry().attach<UIComponent>(m_data.eui1)->setText(textInfo);
	textInfo.data.colour = colours::yellow;
	textInfo.data.pos.y -= 100.0f;
	// textInfo.data.pos.x = 620.0f;
	textInfo.id = "tris";
	registry().attach<UIComponent>(m_data.eui2)->setText(textInfo);
	registry().attach<UIComponent>(m_data.pointer)->setQuad({50.0f, 30.0f}, {25.0f, 15.0f}).material().tint = colours::cyan;

	m_data.freeCam.init();
	m_data.freeCam.m_state.flags.set(FreeCam::Flag::eKeyToggle_Look);
	m_data.freeCam.m_camera.position = {0.0f, 1.0f, 2.0f};

	m_data.eid0.transform().position({1.0f, 1.0f, -2.0f});
	registry().attach<res::Mesh>(m_data.eid0, m_res.quad);

	m_data.eid1.transform().position({0.0f, 0.0f, -2.0f});
	registry().attach<res::Mesh>(m_data.eid1, m_res.sphere);

	m_data.eid2.transform().position({-1.0f, 1.0f, -2.0f}).parent(&m_data.eid1.transform());
	registry().attach<res::Model>(m_data.eid2);

	m_data.eid3.transform().position({0.0f, -1.0f, -3.0f});
	registry().attach<res::Model, Time>(m_data.eid3); // Test attaching multiple components

	m_input.context.mapTrigger("wireframe", [this]() { m_data.bWireframe = !m_data.bWireframe; });
	m_input.context.mapTrigger("reload_models", [this]() { m_data.bLoadUnloadModels = true; });
	m_input.context.mapTrigger("quit", [this]() { m_data.bQuit = true; });
	m_input.context.mapState("run", [](bool bActive) { LOGIF_I(bActive, "RUNNING!"); });
	m_input.context.mapState("pause_cam", [this](bool bActive) { m_data.freeCam.m_state.flags[FreeCam::Flag::eEnabled] = !bActive; });
	m_input.context.addState("pause_cam", input::Key::eLeftControl);
	m_input.context.addState("pause_cam", input::Key::eRightControl);

#if defined(LEVK_DEBUG)
	m_data.temp = {};
	m_data.temp.m_name = "Demo-Temp";
#endif
	m_data.temp.setMode(input::Mode::eBlockAll);
	m_data.temp.mapTrigger("test2", []() { LOG_I("Test2 triggered!"); });
	m_data.temp.addTrigger("test2", input::Key::eK);

	auto& desc = gs::sceneDesc();
	desc.camera = m_data.freeCam.m_camera;
	desc.dirLights = {m_data.dirLight0, m_data.dirLight1};
	desc.clearColour = Colour(0x030203ff);
	desc.skyboxCubemapID = "skyboxes/sky_dusk";
	desc.uiSpace = {1280.0f, 720.0f, 2.0f};
	desc.flags = gs::SceneDesc::Flag::eScissoredUI;

	// for (auto i = 0; i < 1000; ++i)
	// {
	// 	gs::spawnProp(fmt::format("test_{}", i));
	// }
}

void DemoLevel::tick(Time dt)
{
	if (m_data.bQuit)
	{
		engine::Service::shutdown();
		return;
	}

	static Time elapsed;
	elapsed += dt;
	static bool s_bRegistered = false;
	if (elapsed >= 5s && !s_bRegistered)
	{
		m_data.tempToken = input::registerContext(&m_data.temp);
		s_bRegistered = true;
	}
	if (elapsed >= 8s && m_data.tempToken.valid())
	{
		m_data.tempToken = {};
	}

	if (m_data.asyncModel0.loaded())
	{
		if (auto pModel = registry().find<res::Model>(m_data.eid2))
		{
			*pModel = m_data.asyncModel0.resource().payload;
		}
		m_data.asyncModel0 = {};
	}
	if (m_data.asyncModel1.loaded())
	{
		if (auto pModel = registry().find<res::Model>(m_data.eid3))
		{
			*pModel = m_data.asyncModel1.resource().payload;
		}
		m_data.asyncModel1 = {};
	}
	if (m_data.bLoadUnloadModels && !m_data.asyncModel0.valid())
	{
		if (auto model = res::find<res::Model>(m_data.model0id))
		{
			res::unload(*model);
			if (auto model1 = res::find<res::Model>(m_data.model1id))
			{
				res::unload(*model1);
			}
			m_data.asyncModel0 = m_data.asyncModel1 = {};
		}
		else
		{
			res::Model::LoadInfo loadInfo;
			loadInfo.idRoot = loadInfo.jsonDirectory = m_data.model0id;
			m_data.asyncModel0 = res::loadAsync(m_data.model0id, std::move(loadInfo));
			if (m_data.model1id != m_data.model0id)
			{
				loadInfo = {};
				loadInfo.idRoot = loadInfo.jsonDirectory = m_data.model1id;
				m_data.asyncModel1 = res::loadAsync(m_data.model1id, std::move(loadInfo));
			}
		}
		m_data.bLoadUnloadModels = false;
	}

	m_data.freeCam.tick(dt);

	m_fps = m_data.fps.update();
	registry().find<UIComponent>(m_data.eui0)->setText(fmt::format("{}FPS", m_fps));
	registry().find<UIComponent>(m_data.eui1)->setText(fmt::format("{:.3}ms", dt.to_s() * 1000));
	registry().find<UIComponent>(m_data.eui2)->setText(fmt::format("{} entities", registry().size()));
	[[maybe_unused]] auto& quadT = registry().find<Transform>(m_data.pointer)->position({input::worldToUI(input::cursorPosition()), 1.0f});
	// quadT.orientglm::rotate(quadT.orient), glm::radians(dt.to_s() * 50), gfx::g_nFront));

	{
		// Update matrices
		m_data.eid1.transform().orient(glm::rotate(m_data.eid1.transform().orientation(), glm::radians(dt.to_s() * 10), glm::vec3(0.0f, 1.0f, 0.0f)));
		m_data.eid0.transform().orient(glm::rotate(m_data.eid0.transform().orientation(), glm::radians(dt.to_s() * 12), glm::vec3(1.0f, 1.0f, 1.0f)));
		m_data.eid2.transform().orient(glm::rotate(m_data.eid2.transform().orientation(), glm::radians(dt.to_s() * 18), glm::vec3(0.3f, 1.0f, 1.0f)));
	}

	gs::sceneDesc().pipe3D.polygonMode = m_data.bWireframe ? gfx::Pipeline::Polygon::eLine : gfx::Pipeline::Polygon::eFill;
}

SceneBuilder const& DemoLevel::builder() const
{
	return m_sceneBuilder;
}

void DemoLevel::onManifestLoaded()
{
	m_res.sphere.resource.material().diffuse = res::find<res::Texture>(m_res.container2).payload;
	m_res.sphere.resource.material().specular = res::find<res::Texture>(m_res.container2_specular).payload;
	m_res.quad.resource.material().diffuse = res::find<res::Texture>(m_res.awesomeface).payload;
	if (auto pModel = registry().find<res::Model>(m_data.eid3))
	{
		*pModel = res::find<res::Model>(m_data.model1id).payload;
	}
	if (auto pModel = registry().find<res::Model>(m_data.eid2))
	{
		*pModel = res::find<res::Model>(m_data.model0id).payload;
	}
}
