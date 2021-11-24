#pragma once
#include <engine/render/draw_list_factory.hpp>

namespace le {
struct DrawListGen3D {
	// Populates DrawGroup + SceneNode + Prop, DrawGroup + SceneNode + PropProvider, DrawGroup + Skybox
	void operator()(DrawListFactory::GroupMap& map, dens::registry const& registry) const;
};

struct DrawListGenUI {
	// Populates DrawGroup + gui::ViewStack
	void operator()(DrawListFactory::GroupMap& map, dens::registry const& registry) const;
};
} // namespace le
