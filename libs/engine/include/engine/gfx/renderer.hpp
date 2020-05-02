#pragma once
#include <memory>
#include "light.hpp"
#include "pipeline.hpp"

namespace le
{
class Transform;
}

namespace le::gfx
{
class Mesh;

struct ScreenRect final
{
	f32 left = 0.0f;
	f32 top = 0.0f;
	f32 right = 1.0f;
	f32 bottom = 1.0f;

	constexpr ScreenRect() noexcept = default;
	ScreenRect(glm::vec4 const& ltrb) noexcept;
	explicit ScreenRect(glm::vec2 const& size, glm::vec2 const& leftTop = glm::vec2(0.0f)) noexcept;

	f32 aspect() const;
};

class Renderer final
{
public:
	struct ClearValues final
	{
		glm::vec2 depthStencil = {1.0f, 0.0f};
		Colour colour = colours::Black;
	};

	struct Drawable final
	{
		Mesh const* pMesh = nullptr;
		Transform const* pTransform = nullptr;
		Pipeline* pPipeline = nullptr;
	};

	struct Batch final
	{
		ScreenRect viewport;
		ScreenRect scissor;
		std::vector<Drawable> drawables;
	};

	struct View final
	{
		glm::mat4 mat_vp = {};
		glm::mat4 mat_v = {};
		glm::mat4 mat_p = {};
		glm::mat4 mat_ui = {};
		glm::vec3 pos_v = {};
		u32 dirLightCount = 0;
	};

	struct Scene final
	{
		View view;
		ClearValues clear;
		std::vector<Batch> batches;
		std::vector<DirLight> dirLights;
	};

public:
	static std::string const s_tName;

public:
	std::unique_ptr<class RendererImpl> m_uImpl;

public:
	Renderer();
	Renderer(Renderer&&);
	Renderer& operator=(Renderer&&);
	~Renderer();

public:
	Pipeline* createPipeline(Pipeline::Info info);

	void update();
	void render(Scene const& scene);
};
} // namespace le::gfx
