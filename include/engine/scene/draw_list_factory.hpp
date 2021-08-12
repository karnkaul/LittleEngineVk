#pragma once
#include <unordered_map>
#include <core/not_null.hpp>
#include <engine/scene/draw_list.hpp>
#include <engine/scene/layer_hasher.hpp>

namespace decf {
class entity;
class registry;
} // namespace decf

namespace le {
namespace gui {
class TreeRoot;
}

using PropList = Span<Prop const>;

class DrawListFactory {
  public:
	using LayerMap = std::unordered_map<DrawLayer, std::vector<Drawable>, LayerHasher>;

	static void add(LayerMap& map, DrawLayer const& layer, gui::TreeRoot const& root);

	template <typename... Gen>
	static std::vector<DrawList> lists(decf::registry const& registry, bool sort);

	// Attaches DrawLayer, PropList
	static void attach(decf::registry& registry, decf::entity entity, DrawLayer layer, Span<Prop const> props);
};

struct DrawListGen3D {
	// Populates DrawLayer + SceneNode + PropList, DrawLayer + Skybox
	void operator()(DrawListFactory::LayerMap& map, decf::registry const& registry) const;
};

struct DrawListGenUI {
	// Populates DrawLayer + gui::ViewStack
	void operator()(DrawListFactory::LayerMap& map, decf::registry const& registry) const;
};

// impl

template <typename... Gen>
std::vector<DrawList> DrawListFactory::lists(decf::registry const& registry, bool sort) {
	LayerMap map;
	(Gen{}(map, registry), ...);
	std::vector<DrawList> ret;
	ret.reserve(map.size());
	for (auto& [dlayer, list] : map) {
		if (dlayer.pipeline) { ret.push_back(DrawList({dlayer, std::move(list), {}})); }
	}
	if (sort) { std::sort(ret.begin(), ret.end()); }
	return ret;
}
} // namespace le
