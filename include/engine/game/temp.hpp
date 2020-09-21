#pragma once
#include <core/ecs_registry.hpp>
#include <engine/gfx/camera.hpp>
#include <engine/gfx/renderer.hpp>

namespace le
{
namespace foo
{
struct Transform final
{
	glm::vec3 position = {};
	glm::quat orientation = {0.0f, 0.0f, 0.0f, 1.0f};
	glm::vec3 scale = glm::vec3(1.0f);

	bool isotropic() const noexcept;
	glm::mat4 matrix() const noexcept;
	glm::mat4 normals() const noexcept;
};

class GameScene;
} // namespace foo

using ETMap = std::unordered_map<Entity, Transform, EntityHasher>;

struct Temp
{
	bool bUse2 = false;

	virtual gfx::Renderer::Scene build(gfx::Camera const&, Registry const&) const
	{
		return {};
	}
	virtual gfx::Renderer::Scene build2(gfx::Camera const&, ETMap const&, Registry const&, GameScene const&) const
	{
		return {};
	}
};
} // namespace le
