#pragma once
#include <vector>
#include <dumb_ecf/registry.hpp>
#include <graphics/context/command_buffer.hpp>
#include <graphics/pipeline.hpp>

namespace le {
struct DrawLayer {
	graphics::Pipeline* pipeline = {};
	s64 order = 0;
};

template <typename T>
struct DrawList {
	DrawLayer layer;
	std::vector<Ref<T>> ts;
};

template <typename D, typename T = typename std::decay_t<D>::type>
void batchDraw(D&& dispatch, decf::registry_t& out_reg, graphics::CommandBuffer const& cb);

template <typename T>
std::vector<DrawList<T>> drawLists(decf::registry_t& out_registry);

constexpr bool operator==(DrawLayer const& l, DrawLayer const& r) noexcept {
	return l.pipeline == r.pipeline && l.order == r.order;
}

struct LayerHasher {
	std::size_t operator()(DrawLayer const& l) const noexcept {
		return std::hash<graphics::Pipeline const*>{}(l.pipeline) ^ (std::hash<s64>{}(l.order) << 1);
	}
};

template <typename D, typename T>
void batchDraw(D&& dispatch, decf::registry_t& out_reg, graphics::CommandBuffer const& cb) {
	auto lists = drawLists<T>(out_reg);
	for (auto& list : lists) {
		dispatch.update(list);
	}
	std::unordered_set<Ref<graphics::Pipeline>> ps;
	for (auto const& list : lists) {
		ps.insert(*list.layer.pipeline);
		cb.bindPipe(*list.layer.pipeline);
		dispatch.draw(cb, list);
	}
	for (graphics::Pipeline& pipe : ps) {
		pipe.shaderInput().swap();
	}
	dispatch.swap();
}

template <typename T>
std::vector<DrawList<T>> drawLists(decf::registry_t& out_registry) {
	std::unordered_map<DrawLayer, std::vector<Ref<T>>, LayerHasher> map;
	for (auto& [_, d] : out_registry.view<DrawLayer, T>()) {
		auto& [l, t] = d;
		map[l].push_back(t);
	}
	std::vector<DrawList<T>> ret;
	ret.reserve(map.size());
	for (auto& [l, ts] : map) {
		ret.push_back(DrawList<T>({l, std::move(ts)}));
	}
	std::sort(ret.begin(), ret.end(), [](DrawList<T> const& a, DrawList<T> const& b) { return a.layer.order < b.layer.order; });
	return ret;
}
} // namespace le
