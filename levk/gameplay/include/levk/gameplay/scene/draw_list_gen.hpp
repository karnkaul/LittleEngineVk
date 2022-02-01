#pragma once
#include <levk/gameplay/scene/list_renderer.hpp>

namespace le {
struct DrawListGen {
	// Populates DrawGroup + [DynamicMesh, MeshProvider, gui::ViewStack]
	void operator()(ListRenderer::DrawableMap& map, dens::registry const& registry) const;
};

struct DebugDrawListGen {
	inline static bool populate_v = levk_debug;

	// Populates DrawGroup + [physics::Trigger::Debug]
	void operator()(ListRenderer::DrawableMap& map, dens::registry const& registry) const;
};
} // namespace le
