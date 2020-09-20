#pragma once
#include <core/ecs_registry.hpp>
#include <engine/gfx/renderer.hpp>
#include <engine/levk.hpp>

namespace le::gs
{
gfx::Renderer::Scene update(engine::Driver& out_driver, Time dt, bool bTick);

#if defined(LEVK_EDITOR)
using EMap = std::unordered_map<Transform*, Entity>;
EMap& entityMap();
Transform& root();

template <typename Pred>
void walkSceneTree(Transform& root, EMap const& emap, Registry& registry, Pred pred)
{
	auto const children = root.children();
	auto search = emap.find(&root);
	if (search != emap.end())
	{
		auto [pTransform, entity] = *search;
		if (pred(entity, *pTransform))
		{
			for (Transform& child : pTransform->children())
			{
				walkSceneTree(child, emap, registry, pred);
			}
		}
	}
}
#endif
} // namespace le::gs
