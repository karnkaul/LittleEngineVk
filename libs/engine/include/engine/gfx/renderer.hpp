#pragma once
#include <deque>
#include <memory>
#include <vector>
#include "light.hpp"
#include "pipeline.hpp"

namespace le
{
class Transform;
class WindowImpl;
} // namespace le

namespace le::gfx
{
class Mesh;
class Cubemap;

struct ScreenRect final
{
	f32 left = 0.0f;
	f32 top = 0.0f;
	f32 right = 1.0f;
	f32 bottom = 1.0f;

	constexpr ScreenRect() noexcept = default;
	ScreenRect(glm::vec4 const& ltrb) noexcept;

	static ScreenRect sizeTL(glm::vec2 const& size, glm::vec2 const& leftTop = glm::vec2(0.0f));
	static ScreenRect sizeCentre(glm::vec2 const& size, glm::vec2 const& centre = glm::vec2(0.5f));

	glm::vec2 size() const;
	f32 aspect() const;
};

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
		Cubemap const* pCubemap = nullptr;
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
		u32 dirLightCount = 0;
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

public:
	Renderer();
	Renderer(Renderer&&);
	Renderer& operator=(Renderer&&);
	~Renderer();

public:
	Pipeline* createPipeline(Pipeline::Info info);

	void update();
	void render(Scene scene);

	glm::vec2 screenToN(glm::vec2 const& screenXY) const;
	ScreenRect clampToView(glm::vec2 const& screenXY, glm::vec2 const& nViewport, glm::vec2 const& padding = {}) const;

private:
	friend class le::WindowImpl;
};
} // namespace le::gfx
