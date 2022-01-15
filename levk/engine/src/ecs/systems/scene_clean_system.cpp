#include <levk/engine/ecs/systems/scene_clean_system.hpp>
#include <levk/engine/scene/scene_node.hpp>

namespace le {
void SceneCleanSystem::update(dens::registry const& registry) {
	for (auto [_, c] : registry.view<SceneNode>()) {
		auto& [node] = c;
		node.clean(registry);
	}
}
} // namespace le
