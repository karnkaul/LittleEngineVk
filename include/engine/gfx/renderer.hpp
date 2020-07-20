#pragma once
#include <deque>
#include <memory>
#include <vector>
#include <engine/gfx/light.hpp>
#include <engine/gfx/pipeline.hpp>
#include <engine/gfx/screen_rect.hpp>
#include <engine/resources/resource_types.hpp>

namespace le
{
class Transform;
class WindowImpl;
} // namespace le

namespace le::gfx
{
class Mesh;

class Renderer final
{
public:
	struct ClearValues final
	{
		glm::vec2 depthStencil = {1.0f, 0.0f};
		Colour colour = colours::black;
	};

	struct Skybox final
	{
		ScreenRect viewport;
		resources::Texture cubemap;
		Pipeline const* pPipeline = nullptr;
	};

	struct Drawable final
	{
		std::vector<Mesh const*> meshes;
		Transform const* pTransform = nullptr;
		Pipeline const* pPipeline = nullptr;
	};

	struct Batch final
	{
		ScreenRect viewport;
		ScreenRect scissor;
		std::deque<Drawable> drawables;
	};

	struct View final
	{
		glm::mat4 mat_vp = {};
		glm::mat4 mat_v = {};
		glm::mat4 mat_p = {};
		glm::mat4 mat_ui = {};
		glm::vec3 pos_v = {};
		Skybox skybox;
	};

	struct Scene final
	{
		View view;
		ClearValues clear;
		std::deque<Batch> batches;
		std::vector<DirLight> dirLights;
	};

	struct Stats final
	{
		u64 trisDrawn = 0;
	};

public:
	static std::string const s_tName;

public:
	Stats m_stats;

private:
	std::unique_ptr<class RendererImpl> m_uImpl;
	Scene m_scene;
	ScreenRect m_sceneView;

public:
	Renderer();
	Renderer(Renderer&&);
	Renderer& operator=(Renderer&&);
	~Renderer();

public:
	Pipeline* createPipeline(Pipeline::Info info);

	void submit(Scene scene, ScreenRect const& sceneView);

	glm::vec2 screenToN(glm::vec2 const& screenXY) const;
	ScreenRect clampToView(glm::vec2 const& screenXY, glm::vec2 const& nViewport, glm::vec2 const& padding = {}) const;

private:
	void render(bool bEditor);

private:
	friend class le::WindowImpl;
	friend class RendererImpl;
};
} // namespace le::gfx
