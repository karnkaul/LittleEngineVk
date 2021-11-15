#pragma once
#include <core/not_null.hpp>
#include <engine/render/draw_list.hpp>
#include <engine/render/layer_hasher.hpp>
#include <unordered_map>

namespace dens {
struct entity;
class registry;
} // namespace dens

namespace le {
namespace gui {
class TreeRoot;
}

using PropList = Span<Prop const>;

class DrawListFactory {
  public:
	using LayerMap = std::unordered_map<DrawLayer, std::vector<Drawable>, LayerHasher>;

	template <typename... Gen>
	static std::vector<DrawList> lists(dens::registry const& registry, bool sort);
};

// impl

template <typename... Gen>
std::vector<DrawList> DrawListFactory::lists(dens::registry const& registry, bool sort) {
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
