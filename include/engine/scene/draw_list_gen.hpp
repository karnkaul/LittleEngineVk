#pragma once
#include <engine/render/draw_list_factory.hpp>

namespace le {
struct DrawListGen {
	// Populates DrawGroup + [DynamicMesh, MeshProvider, gui::ViewStack]
	void operator()(DrawListFactory::GroupMap& map, dens::registry const& registry) const;
};

struct DebugDrawListGen {
	inline static bool populate_v = levk_debug;

	// Populates DrawGroup + [physics::Trigger::Debug]
	void operator()(DrawListFactory::GroupMap& map, dens::registry const& registry) const;
};
} // namespace le
