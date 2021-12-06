#pragma once
#include <engine/render/list_drawer.hpp>

namespace le {
struct DrawListGen {
	// Populates DrawGroup + [DynamicMesh, MeshProvider, gui::ViewStack]
	void operator()(ListDrawer::LayerMap& map, dens::registry const& registry) const;
};

struct DebugDrawListGen {
	inline static bool populate_v = levk_debug;

	// Populates DrawGroup + [physics::Trigger::Debug]
	void operator()(ListDrawer::LayerMap& map, dens::registry const& registry) const;
};
} // namespace le
