#pragma once
#include <unordered_map>
#include <core/not_null.hpp>
#include <engine/scene/draw_list.hpp>
#include <engine/scene/layer_hasher.hpp>

namespace decf {
class entity_t;
class registry_t;
} // namespace decf

namespace le {
namespace gui {
class TreeRoot;
}

using PrimitiveList = Span<Primitive const>;

class DrawListFactory {
  public:
	using LayerMap = std::unordered_map<DrawLayer, std::vector<Drawable>, LayerHasher>;

	static void add(LayerMap& map, DrawLayer const& layer, gui::TreeRoot const& root);

	template <typename... Gen>
	static std::vector<DrawList> lists(decf::registry_t const& registry, bool sort);

	// Attaches DrawLayer, PrimitiveList
	static void attach(decf::registry_t& registry, decf::entity_t entity, DrawLayer layer, Span<Primitive const> primitives);
};

struct DrawListGen3D {
	// Populates DrawLayer + SceneNode + PrimitiveList
	void operator()(DrawListFactory::LayerMap& map, decf::registry_t const& registry) const;
};

struct DrawListGenUI {
	// Populates DrawLayer + gui::ViewStack
	void operator()(DrawListFactory::LayerMap& map, decf::registry_t const& registry) const;
};

// impl

template <typename... Gen>
std::vector<DrawList> DrawListFactory::lists(decf::registry_t const& registry, bool sort) {
	LayerMap map;
	(Gen{}(map, registry), ...);
	std::vector<DrawList> ret;
	ret.reserve(map.size());
	for (auto& [gr, list] : map) { ret.push_back(DrawList({gr, std::move(list)})); }
	if (sort) { std::sort(ret.begin(), ret.end()); }
	return ret;
}
} // namespace le
