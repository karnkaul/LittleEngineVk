#include <engine/scene/scene_node.hpp>
#include <engine/systems/scene_clean_system.hpp>

namespace le {
void SceneCleanSystem::tick(dens::registry const& registry, Time_s) {
	for (auto [_, c] : registry.view<SceneNode>()) {
		auto& [node] = c;
		node.clean(registry);
	}
}
} // namespace le
