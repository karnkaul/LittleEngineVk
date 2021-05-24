#pragma once
#include <unordered_set>
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <dumb_ecf/types.hpp>
#include <engine/scene/primitive.hpp>
#include <glm/mat4x4.hpp>
#include <graphics/context/command_buffer.hpp>
#include <graphics/descriptor_set.hpp>
#include <graphics/pipeline.hpp>

namespace decf {
class registry_t;
}
namespace le {
namespace gui {
class Root;
}

using PrimList = std::vector<Primitive>;

struct DrawGroup {
	graphics::Pipeline* pipeline = {};
	s64 order = 0;

	struct Hasher {
		std::size_t operator()(DrawGroup const& gr) const noexcept;
	};
};

constexpr bool operator==(DrawGroup const& l, DrawGroup const& r) noexcept { return l.pipeline == r.pipeline && l.order == r.order; }

class SceneDrawer {
  public:
	struct Item {
		glm::mat4 model = glm::mat4(1.0f);
		std::optional<vk::Rect2D> scissor;
		View<Primitive> primitives;
	};

	using ItemMap = std::unordered_map<DrawGroup, std::vector<Item>, DrawGroup::Hasher>;

	struct Group {
		DrawGroup group;
		std::vector<Item> items;
	};

	struct Populator {
		// Populates DrawGroup + SceneNode + PrimList, DrawGroup + gui::Root
		void operator()(ItemMap& map, decf::registry_t const& registry) const;
	};

	static void add(ItemMap& map, DrawGroup const& group, gui::Root const& root);

	static void sort(Span<Group> items) noexcept;
	template <typename Po = Populator>
	static std::vector<Group> groups(decf::registry_t const& registry, bool sort);
	template <typename Di, typename Po = Populator>
	static void draw(Di&& dispatch, View<Group> groups, graphics::CommandBuffer const& cb);

	static void attach(decf::registry_t& reg, decf::entity_t entity, DrawGroup const& group, View<Primitive> primitives);
};

// impl

template <typename Po>
std::vector<SceneDrawer::Group> SceneDrawer::groups(decf::registry_t const& registry, bool sort) {
	ItemMap map;
	Po{}(map, registry);
	std::vector<Group> ret;
	ret.reserve(map.size());
	for (auto& [gr, items] : map) { ret.push_back(Group({gr, std::move(items)})); }
	if (sort) { SceneDrawer::sort(ret); }
	return ret;
}

template <typename Di, typename Po>
void SceneDrawer::draw(Di&& dispatch, View<Group> groups, graphics::CommandBuffer const& cb) {
	std::unordered_set<graphics::Pipeline*> ps;
	for (auto const& gr : groups) {
		if (gr.group.pipeline) {
			ps.insert(gr.group.pipeline);
			cb.bindPipe(*gr.group.pipeline);
			dispatch.draw(cb, gr);
		}
	}
	for (graphics::Pipeline* pipe : ps) { pipe->shaderInput().swap(); }
}
} // namespace le
