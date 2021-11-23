#pragma once
#include <engine/render/draw_list_factory.hpp>

namespace le {
struct DrawListGen3D {
	// Populates DrawLayer + SceneNode + Prop, DrawLayer + SceneNode + PropProvider, DrawLayer + Skybox
	void operator()(DrawListFactory::LayerMap& map, dens::registry const& registry) const;
};

struct DrawListGenUI {
	// Populates DrawLayer + gui::ViewStack
	void operator()(DrawListFactory::LayerMap& map, dens::registry const& registry) const;
};

struct DrawListGen3D2 {
	// Populates DrawGroup + SceneNode + Prop, DrawGroup + SceneNode + PropProvider, DrawGroup + Skybox
	void operator()(DrawListFactory::GroupMap& map, dens::registry const& registry) const;
};

struct DrawListGenUI2 {
	// Populates DrawGroup + gui::ViewStack
	void operator()(DrawListFactory::GroupMap& map, dens::registry const& registry) const;
};
} // namespace le
