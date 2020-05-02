#include "core/assert.hpp"
#include "core/io.hpp"
#include "core/jobs.hpp"
#include "core/log.hpp"
#include "core/maths.hpp"
#include "core/map_store.hpp"
#include "core/transform.hpp"
#include "engine/levk.hpp"
#include "engine/assets/resources.hpp"
#include "engine/gfx/camera.hpp"
#include "engine/gfx/geometry.hpp"
#include "engine/gfx/light.hpp"
#include "engine/gfx/mesh.hpp"
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
	engine::Service engine;
	if (!engine.start(argc, argv))
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
	auto pShader = g_pResources->create<gfx::Shader>("shaders/uber", tutorialShaderInfo);

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
	meshInfo.geometry = gfx::createCone();
	gfx::Mesh* pMesh0 = g_pResources->create<gfx::Mesh>("mesh0", meshInfo);
	meshInfo.geometry = gfx::createCubedSphere(1.0f, 8);
	gfx::Mesh* pMesh1 = g_pResources->create<gfx::Mesh>("mesh1", meshInfo);

	gfx::Material::Info texturedInfo;
	texturedInfo.albedo.ambient = Colour(0x888888ff);
	auto pTexturedLit = g_pResources->create<gfx::Material>("materials/textured", texturedInfo);

	gfx::Texture::Info textureInfo;
	textureInfo.pReader = g_uReader.get();
	pMesh0->m_material.flags.set({gfx::Material::Flag::eTextured, gfx::Material::Flag::eLit, gfx::Material::Flag::eOpaque});
	pMesh0->m_material.pMaterial = pTexturedLit;
	pMesh0->m_material.tint.a = 0x88;
	pMesh1->m_material.flags.set({gfx::Material::Flag::eTextured, gfx::Material::Flag::eLit, gfx::Material::Flag::eOpaque});
	pMesh1->m_material.pMaterial = pTexturedLit;
	pMesh1->m_material.tint.a = 0xcc;
	textureInfo.assetID = "textures/container2.png";
	pMesh1->m_material.pDiffuse = pMesh0->m_material.pDiffuse = g_pResources->create<gfx::Texture>(textureInfo.assetID, textureInfo);
	textureInfo.assetID = "textures/container2_specular.png";
	pMesh1->m_material.pSpecular = pMesh0->m_material.pSpecular = g_pResources->create<gfx::Texture>(textureInfo.assetID, textureInfo);

	gfx::DirLight dirLight0;
	dirLight0.diffuse = Colour(0x0000ffff);
	dirLight0.direction = glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f));
	gfx::DirLight dirLight1;
	dirLight1.diffuse = Colour(0xff00ffff);
	dirLight1.direction = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

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
	info1.options.colourSpaces = {ColourSpace::eRGBLinear};
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
	bool bTEMP = false;
	wf0Token = w0.registerInput([&bWF0, &bTEMP](Key key, Action action, Mods mods) {
		if (key == Key::eP && action == Action::eRelease && mods & Mods::eCONTROL)
		{
			bWF0 = !bWF0;
		}
		if (key == Key::eS && action == Action::eRelease && mods & Mods::eCONTROL)
		{
			bTEMP = !bTEMP;
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
	gfx::FreeCam freeCam(&w0);
	if (w1.create(info1) && w0.create(info0))
	{
		gfx::Pipeline *pPipeline0 = nullptr, *pPipeline0wf = nullptr, *pPipeline1 = nullptr;

		if (w0.isOpen())
		{
			auto& renderer0 = w0.renderer();
			pPipeline0 = createPipeline(&renderer0, "default");
			pPipeline0wf = createPipeline(&renderer0, "wireframe", gfx::PolygonMode::eLine);
		}
		if (w1.isOpen())
		{
			auto& renderer1 = w1.renderer();
			pPipeline1 = createPipeline(&renderer1, "default");
		}

		gfx::Renderer::View view0;
		gfx::Renderer::View view1;
		freeCam.m_position = {0.0f, 1.0f, 2.0f};
		Transform transform0, transform01;
		Transform transform1;
		transform0.setPosition({0.0f, 0.0f, -2.0f});
		transform01.setPosition({1.0f, 1.0f, -2.0f});

		Time t = Time::elapsed();
		while (w0.isOpen() || w1.isOpen())
		{
			[[maybe_unused]] Time dt = Time::elapsed() - t;
			t = Time::elapsed();

			static Time fpsLogTime = Time::from_s(3.0f);
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
			if (fpsLogElapsed >= fpsLogTime)
			{
				LOG_I("dt: {}, FPS: {}", dt.to_s() * 1000, fps);
				fps = 0;
				fpsLogElapsed = Time();
			}

			{
				g_pResources->update();
				w0.renderer().update();
				w1.renderer().update();

				freeCam.tick(dt);

				// Update matrices
				transform0.setOrientation(glm::rotate(transform0.orientation(), glm::radians(dt.to_s() * 10), glm::vec3(0.0f, 1.0f, 0.0f)));
				transform01.setOrientation(
					glm::rotate(transform01.orientation(), glm::radians(dt.to_s() * 12), glm::vec3(1.0f, 1.0f, 1.0f)));
				transform1.setOrientation(glm::rotate(transform1.orientation(), glm::radians(dt.to_s() * 15), glm::vec3(0.0f, 1.0f, 0.0f)));
				view0.mat_v = freeCam.view();
				view0.pos_v = freeCam.m_position;
				view1.mat_v = glm::lookAt(view0.pos_v, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				view1.pos_v = freeCam.m_position;
				if (w0.isOpen())
				{
					auto const size = w0.framebufferSize();
					if (size.x > 0 && size.y > 0)
					{
						view0.mat_p = freeCam.perspectiveProj((f32)size.x / (f32)size.y);
						view0.mat_vp = view0.mat_p * view0.mat_v;
					}
				}
				if (w1.isOpen())
				{
					auto const size = w1.framebufferSize();
					if (size.x > 0 && size.y > 0)
					{
						view1.mat_p = glm::perspective(glm::radians(45.0f), (f32)size.x / (f32)size.y, 0.1f, 10.0f);
						view1.mat_vp = view1.mat_p * view1.mat_v;
					}
				}
			}

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
				w0.create(info0);
				pPipeline0 = createPipeline(&w0.renderer(), "default");
				pPipeline0wf = createPipeline(&w0.renderer(), "wireframe", gfx::PolygonMode::eLine);
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
					if (pMesh0->isReady() && pTriangle0->isReady())
					{
						auto pPipeline = bWF0 ? pPipeline0wf : pPipeline0;
						gfx::Renderer::Batch batch;
						batch.drawables = {{pMesh0, &transform01, pPipeline}, {pMesh1, &transform0, pPipeline}};
						if (bTEMP)
						{
							batch.drawables.push_back({pTriangle0, &transform0, pPipeline});
						}
						scene.batches.push_back(std::move(batch));
						w0.renderer().render(scene);
					}
				}
				if (w1.isOpen() && pTriangle0->isReady())
				{
					gfx::Renderer::Scene scene;
					scene.dirLights.push_back(dirLight0);
					scene.clear.colour = Colour(0x030203ff);
					scene.view = view1;
					if (pTriangle0->isReady())
					{
						gfx::Renderer::Batch batch;
						batch.drawables = {{pTriangle0, &transform0, pPipeline1}};
						scene.batches.push_back(std::move(batch));
						w1.renderer().render(scene);
					}
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
