#pragma once
#include <compare>
#include <unordered_set>
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <dumb_ecf/types.hpp>
#include <engine/scene/primitive.hpp>
#include <glm/mat4x4.hpp>
#include <graphics/render/command_buffer.hpp>
#include <graphics/render/descriptor_set.hpp>
#include <graphics/render/pipeline.hpp>

namespace decf {
class registry_t;
}
namespace le {
namespace gui {
class TreeRoot;
}

using PrimList = std::vector<Primitive>;

struct DrawGroup {
	graphics::Pipeline* pipeline = {};
	s64 order = 0;

	constexpr bool operator==(DrawGroup const& rhs) const noexcept = default;
	constexpr auto operator<=>(DrawGroup const& rhs) const noexcept { return order <=> rhs.order; }

	struct Hasher {
		std::size_t operator()(DrawGroup const& gr) const noexcept;
	};
};

class SceneDrawer {
  public:
	struct Item {
		glm::mat4 model = glm::mat4(1.0f);
		std::optional<vk::Rect2D> scissor;
		Span<Primitive const> primitives;
	};

	using ItemMap = std::unordered_map<DrawGroup, std::vector<Item>, DrawGroup::Hasher>;

	struct Group {
		DrawGroup group;
		std::vector<Item> items;

		constexpr bool operator==(Group const& rhs) const noexcept { return group == rhs.group; }
		constexpr auto operator<=>(Group const& rhs) const noexcept { return group <=> rhs.group; }
	};

	using PipeSet = std::unordered_set<graphics::Pipeline*>;

	static void add(ItemMap& map, DrawGroup const& group, gui::TreeRoot const& root);

	template <typename... Populator>
	static std::vector<Group> groups(decf::registry_t const& registry, bool sort);

	template <typename Di>
	static void draw(Di&& dispatch, PipeSet& out_set, Span<Group const> groups, graphics::CommandBuffer cb);

	static void attach(decf::registry_t& reg, decf::entity_t entity, DrawGroup const& group, Span<Primitive const> primitives);
};

struct ScenePopulator3D {
	// Populates DrawGroup + SceneNode + PrimList
	void operator()(SceneDrawer::ItemMap& map, decf::registry_t const& registry) const;
};

struct ScenePopulatorUI {
	// Populates DrawGroup + gui::ViewStack
	void operator()(SceneDrawer::ItemMap& map, decf::registry_t const& registry) const;
};

// impl

template <typename... Populator>
std::vector<SceneDrawer::Group> SceneDrawer::groups(decf::registry_t const& registry, bool sort) {
	ItemMap map;
	(Populator{}(map, registry), ...);
	std::vector<Group> ret;
	ret.reserve(map.size());
	for (auto& [gr, items] : map) { ret.push_back(Group({gr, std::move(items)})); }
	if (sort) { std::sort(ret.begin(), ret.end()); }
	return ret;
}

template <typename Di>
void SceneDrawer::draw(Di&& dispatch, PipeSet& out_set, Span<Group const> groups, graphics::CommandBuffer cb) {
	for (auto const& gr : groups) {
		if (gr.group.pipeline) {
			out_set.insert(gr.group.pipeline);
			cb.bindPipe(*gr.group.pipeline);
			dispatch.draw(cb, gr);
		}
	}
}
} // namespace le
