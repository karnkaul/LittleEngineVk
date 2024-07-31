#pragma once
#include <levk/io/json_io.hpp>
#include <levk/node_tree.hpp>
#include <cassert>
#include <ranges>
#include <unordered_map>

namespace levk {
template <std::derived_from<TreeNode> NodeT = TreeNode, typename TreeT = BasicNodeTree<NodeT>>
class TreeToJson {
  public:
	struct NullPerNode {
		constexpr void operator()(NodeT const& /*in*/, dj::Json& /*out*/) const {}
	};

	template <typename PerNode = NullPerNode>
	[[nodiscard]] auto export_tree(TreeT const& source, PerNode const& func = {}) const -> dj::Json {
		auto ret = dj::Json{};
		m_source = &source;
		for (auto const id : source.get_root_ids()) {
			auto const* root_node = source.get_node(id);
			assert(root_node);
			export_node(ret, *root_node, {}, func);
		}
		m_source = {};
		return ret;
	}

  private:
	template <typename PerNode = NullPerNode>
	// NOLINTNEXTLINE(misc-no-recursion)
	auto export_node(dj::Json& out_nodes, NodeT const& in, std::optional<std::size_t> parent, PerNode const& per_node) const -> std::size_t {
		auto const ret = out_nodes.array_view().size();
		// pointer because it needs to be rebound if children are added
		auto* out_node = &out_nodes.push_back({});
		for (auto const& child_id : in.get_children_ids()) {
			auto const* child_node = m_source->get_node(child_id);
			assert(child_node);
			auto const child_index = export_node(out_nodes, *child_node, ret, per_node);
			// repoint as vector may have resized
			assert(out_nodes.array_view().size() > ret);
			out_node = &out_nodes[ret];
			(*out_node)["children"].push_back(child_index);
		}
		(*out_node)["name"] = in.name;
		to_json((*out_node)["transform"], in.transform.get_data());
		if (auto const import_index = in.get_import_index(); import_index > ImportIndex::eNone) {
			(*out_node)["import_index"] = static_cast<std::int64_t>(import_index);
		}
		if (parent) { (*out_node)["parent"] = *parent; }
		per_node(in, *out_node);
		return ret;
	}

	mutable Ptr<TreeT const> m_source{};
};

template <std::derived_from<TreeNode> NodeT = TreeNode, typename TreeT = BasicNodeTree<NodeT>>
class JsonToTree {
  public:
	struct NullPerNode {
		constexpr void operator()(dj::Json const& /*in*/, NodeT& /*out*/) const {}
	};

	template <typename PerNode = NullPerNode>
	[[nodiscard]] auto import_tree(dj::Json const& json, PerNode per_node = {}) -> TreeT {
		auto ret = TreeT{};
		m_index_to_id.clear();
		for (auto const& [index, in] : std::ranges::enumerate_view(json.array_view())) {
			auto node = NodeT{};
			node.name = in["name"].as_string();
			auto transform = Transform::Data{};
			from_json(in["transform"], transform);
			node.transform.set_data(transform);
			auto const& import_index = in["import_index"].template as<std::int64_t>(static_cast<std::int64_t>(ImportIndex::eNone));
			auto& out_node = ret.add_node(std::move(node), ImportIndex{import_index});
			m_index_to_id.insert_or_assign(index, out_node.get_id());
		}
		for (auto const& [index, in] : std::ranges::enumerate_view(json.array_view())) {
			auto* node = ret.get_node(m_index_to_id[index]);
			assert(node);
			if (auto const& parent_index = in["parent"]) { ret.set_parent(*node, m_index_to_id[parent_index.template as<std::int64_t>()]); }
			per_node(in, *node);
		}
		m_index_to_id.clear();
		return ret;
	}

  private:
	std::unordered_map<std::int64_t, TreeNodeId> m_index_to_id{};
};
} // namespace levk
