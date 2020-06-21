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
#include <engine/assets/manifest.hpp>
#include <engine/assets/resources.hpp>
#include <engine/ecs/registry.hpp>
#include <engine/game/scene_builder.hpp>
#include <engine/game/world.hpp>
#include <engine/gfx/camera.hpp>
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

class DemoWorld : public World
{
public:
	u32 m_fps;

private:
	struct
	{
		gfx::Model::LoadRequest modelLoadReq;
		stdfs::path model0id, model1id;
		gfx::Renderer::View view;
		gfx::FreeCam freeCam;
		gfx::DirLight dirLight0, dirLight1;
		Entity eid0, eid1, eid2, eid3;
		Entity eui0, eui1, eui2;
		OnInput::Token inputToken;
		Time reloadTime;
		bool bLoadUnloadModels = false;
		bool bWireframe = false;
		bool bDisableCam = false;
		bool bQuit = false;
	} m_data;
	struct
	{
		gfx::Texture* pCubemap = nullptr;
		gfx::Mesh* pTriangle0 = nullptr;
		gfx::Mesh* pQuad = nullptr;
		gfx::Mesh* pSphere = nullptr;
		gfx::Model* pModel0 = nullptr;
		gfx::Model* pModel1 = nullptr;
	} m_res;
	gfx::Pipeline* m_pPipeline0wf = nullptr;
	std::unique_ptr<AssetManifest> m_uManifest;

protected:
	bool start() override;
	void tick(Time dt) override;
	gfx::Renderer::Scene buildScene() const override;
	void stop() override;
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

	gfx::Model::LoadRequest model0lr;
	model0lr.jsonID = g_uReader->checkPresence("models/test/nanosuit/nanosuit.json") ? "models/test/nanosuit" : "models/plant";
	model0lr.pReader = g_uReader.get();
	m_data.model0id = model0lr.getModelID();
	m_data.model0id += "_0";
	jobs::enqueue([this, model0lr]() {
		auto semaphore = Resources::inst().setBusy();
		auto model0info = gfx::Model::parseOBJ(model0lr);
		if (auto pModel = m_registry.component<TAsset<gfx::Model>>(m_data.eid2))
		{
			pModel->id = m_data.model0id;
		}
		m_res.pModel0 = Resources::inst().create<gfx::Model>(m_data.model0id, std::move(model0info));
	});
	m_data.model1id = model0lr.getModelID();
	m_data.model1id += "_1";
	jobs::enqueue([this, model0lr]() {
		auto semaphore = Resources::inst().setBusy();
		auto model1info = gfx::Model::parseOBJ(model0lr);
		if (auto pModel = m_registry.component<TAsset<gfx::Model>>(m_data.eid3))
		{
			pModel->id = m_data.model1id;
		}
		model1info.mode = gfx::Texture::Space::eRGBLinear;
		m_res.pModel1 = Resources::inst().create<gfx::Model>(m_data.model1id, std::move(model1info));
	});
	gfx::Material::Info texturedInfo;
	texturedInfo.albedo.ambient = Colour(0x888888ff);
	auto pTexturedLit = Resources::inst().create<gfx::Material>("materials/textured", texturedInfo);

	m_uManifest = std::make_unique<AssetManifest>(*g_uReader, "demo.manifest");
	m_uManifest->start();
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

	m_data.modelLoadReq.jsonID = g_uReader->checkPresence("models/test/nanosuit/nanosuit.json") ? "models/test/nanosuit" : "models/plant";
	m_data.modelLoadReq.pReader = g_uReader.get();

	m_data.eid0 = spawnEntity("quad");
	m_data.eid1 = spawnEntity("sphere");
	m_data.eid2 = spawnEntity("model0");
	m_data.eid3 = spawnEntity("model1");
	m_data.eui0 = spawnEntity("fps");
	m_data.eui1 = spawnEntity("dt", false);
	m_data.eui2 = spawnEntity("tris", false);

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
	textInfo.id = "tris";
	m_registry.addComponent<UIComponent>(m_data.eui2)->setText(textInfo);

	m_data.freeCam.init(window());
	m_data.freeCam.m_position = {0.0f, 1.0f, 2.0f};

	m_data.inputToken = Window::registerInput(
		[this](Key key, Action action, Mods mods) {
			if (key == Key::eW && action == Action::eRelease && mods & Mods::eCONTROL)
			{
				m_data.bQuit = true;
			}
			if ((key == Key::eLeftControl || key == Key::eRightControl) && (action == Action::eRelease || action == Action::ePress))
			{
				m_data.bDisableCam = action == Action::ePress;
			}
			if (key == Key::eP && action == Action::eRelease && (mods & Mods::eCONTROL))
			{
				m_data.bWireframe = !m_data.bWireframe;
			}
			if (key == Key::eM && action == Action::eRelease && (mods & Mods::eCONTROL))
			{
				m_data.bLoadUnloadModels = true;
			}
		},
		{});

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
		m_pPipeline0wf = window()->renderer().createPipeline(std::move(pipelineInfo));
	}

	return true;
}

void DemoWorld::tick(Time dt)
{
	if (m_uManifest && m_uManifest->update() == AssetManifest::Status::eIdle)
	{
		m_res.pSphere->m_material.pDiffuse = Resources::inst().get<gfx::Texture>("textures/container2.png");
		m_res.pSphere->m_material.pSpecular = Resources::inst().get<gfx::Texture>("textures/container2_specular.png");
		m_res.pQuad->m_material.pDiffuse = Resources::inst().get<gfx::Texture>("textures/awesomeface.png");
		m_res.pCubemap = Resources::inst().get<gfx::Texture>("skyboxes/sky_dusk");
		m_uManifest.reset();
	}
	if (m_data.bQuit)
	{
		window()->close();
	}
	if (window()->isClosing())
	{
		m_data.freeCam.reset(false, false);
		m_uManifest.reset();
		window()->destroy();
		return;
	}

	if (m_data.bLoadUnloadModels)
	{
		if (m_res.pModel0 && m_res.pModel1)
		{
			Resources::inst().unload<gfx::Model>(m_res.pModel0->m_id);
			Resources::inst().unload<gfx::Model>(m_res.pModel1->m_id);
			m_res.pModel0 = nullptr;
			m_res.pModel1 = nullptr;
		}
		else
		{
			m_data.reloadTime = Time::elapsed();
			jobs::enqueue(
				[this]() {
					auto semaphore = Resources::inst().setBusy();
					auto m0info = gfx::Model::parseOBJ(m_data.modelLoadReq);
					LOG_I("{} data loaded in: {}s", m0info.id.generic_string(), (Time::elapsed() - m_data.reloadTime).to_s());
					m_res.pModel0 = Resources::inst().create<gfx::Model>(m_data.model0id, std::move(m0info));
					LOG_I("{} total load time: {}s", m0info.id.generic_string(), (Time::elapsed() - m_data.reloadTime).to_s());
				},
				"Model0-Reload");
			jobs::enqueue(
				[this]() {
					auto semaphore = Resources::inst().setBusy();
					auto m1info = gfx::Model::parseOBJ(m_data.modelLoadReq);
					LOG_I("{} data loaded in: {}s", m1info.id.generic_string(), (Time::elapsed() - m_data.reloadTime).to_s());
					m1info.mode = gfx::Texture::Space::eRGBLinear;
					m_res.pModel1 = Resources::inst().create<gfx::Model>(m_data.model1id, std::move(m1info));
					LOG_I("{} total load time: {}s", m1info.id.generic_string(), (Time::elapsed() - m_data.reloadTime).to_s());
				},
				"Model1-Reload");
		}
		m_data.bLoadUnloadModels = false;
	}

	m_data.freeCam.m_state.flags[gfx::FreeCam::Flag::eEnabled] = !m_data.bDisableCam;
	m_data.freeCam.tick(dt);

	m_registry.component<UIComponent>(m_data.eui0)->uText->updateText(fmt::format("{}FPS", m_fps));
	m_registry.component<UIComponent>(m_data.eui1)->uText->updateText(fmt::format("{:.3}ms", dt.to_s() * 1000));
	m_registry.component<UIComponent>(m_data.eui2)->uText->updateText(fmt::format("{} triangles", window()->renderer().m_stats.trisDrawn));

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

	m_data.view.mat_v = m_data.freeCam.view();
	m_data.view.pos_v = m_data.freeCam.m_position;
	m_data.view.skybox.pCubemap = m_res.pCubemap;
	auto const size = window()->framebufferSize();
	if (size.x > 0 && size.y > 0)
	{
		m_data.view.mat_ui = m_data.freeCam.ui({size, 2.0f});
		m_data.view.mat_p = m_data.freeCam.perspective((f32)size.x / (f32)size.y);
		m_data.view.mat_vp = m_data.view.mat_p * m_data.view.mat_v;
	}
}

gfx::Renderer::Scene DemoWorld::buildScene() const
{
	SceneBuilder builder;
	builder.info.dirLights = {m_data.dirLight0, m_data.dirLight1};
	builder.info.clearValues.colour = Colour(0x030203ff);
	builder.info.view = m_data.view;
	builder.info.p3Dpipe = m_data.bWireframe ? m_pPipeline0wf : nullptr;
	builder.info.pSkybox = Resources::inst().get<gfx::Texture>("cubemaps/sky_dusk");
	return builder.build(m_registry);
}

void DemoWorld::stop()
{
	if (m_uManifest)
	{
		m_uManifest->update(true);
	}
	m_data = {};
	auto unload = [](std::initializer_list<stdfs::path const*> ids) {
		for (auto pID : ids)
		{
			Resources::inst().unload(*pID);
		}
	};
	auto unloadModels = [&unload](std::initializer_list<gfx::Model const*> models) {
		for (auto pModel : models)
		{
			if (pModel)
			{
				unload({&pModel->m_id});
			}
		}
	};
	if (m_res.pCubemap)
	{
		unload({&m_res.pCubemap->m_id});
	}
	unload({&m_res.pQuad->m_id, &m_res.pSphere->m_id, &m_res.pTriangle0->m_id});
	unloadModels({m_res.pModel0, m_res.pModel1});
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
	info0.config.centreOffset = {-200, -200};
	info.windowInfo = info0;
	info.pReader = g_uReader.get();
	info.dataPaths = engine.locateData({{{"data"}}, {{"demo/data"}}});
	if (!engine.init(info))
	{
		return 1;
	}

	World::ID worldID = World::addWorld<DemoWorld>();
	auto pWorld = World::getWorld<DemoWorld>();

	World::start(worldID);

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

#if defined(LEVK_EDITOR)
			// if (editor::g_bTickGame)
#endif
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
