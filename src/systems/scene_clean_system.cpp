#include <engine/scene/scene_node.hpp>
#include <engine/systems/scene_clean_system.hpp>

namespace le {
void SceneCleanSystem::update(dens::registry const& registry) {
	for (auto [_, c] : registry.view<SceneNode>()) {
		auto& [node] = c;
		node.clean(registry);
	}
}
} // namespace le
