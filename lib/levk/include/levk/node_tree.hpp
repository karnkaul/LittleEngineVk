#pragma once
#include <levk/core/not_null.hpp>
#include <levk/core/polymorphic.hpp>
#include <levk/import_index.hpp>
#include <levk/transform.hpp>
#include <unordered_map>
#include <vector>

namespace levk {
/// \brief Type to identify a node within a tree.
enum struct TreeNodeId : int { eNone = 0 };

/// \brief Convert an integer to an ImportIndex.
template <std::integral T>
constexpr auto to_import_index(T const index) {
	return static_cast<ImportIndex>(index);
}

constexpr auto to_size_t(ImportIndex const in) { return static_cast<std::size_t>(in); }

template <typename Type>
class BasicNodeTree;

/// \brief Base class for tree nodes.
class TreeNode : public Polymorphic {
  public:
	using Id = TreeNodeId;

	[[nodiscard]] auto get_id() const -> Id { return m_id; }

	[[nodiscard]] auto has_parent() const -> bool { return m_parent_id > Id::eNone; }
	[[nodiscard]] auto get_parent_id() const -> Id { return m_parent_id; }

	[[nodiscard]] auto get_children_ids() const -> std::span<Id const> { return m_children; }

	[[nodiscard]] auto get_import_index() const -> ImportIndex { return m_import_index; }

	Transform transform{};
	std::string name{"unnamed"};

  private:
	Id m_id{};
	Id m_parent_id{};
	ImportIndex m_import_index{ImportIndex::eNone};

	std::vector<Id> m_children{};

	template <typename Type>
	friend class BasicNodeTree;
};

using NodeTree = BasicNodeTree<TreeNode>;

/// \brief Base class template for node trees.
template <typename Type>
class BasicNodeTree : public Polymorphic {
  public:
	static_assert(std::derived_from<Type, TreeNode>);

	using Id = TreeNodeId;
	using node_type_t = Type;
	using NodeCache = std::vector<NotNull<Type*>>;

	/// \brief Add a node.
	/// \param node Node to add.
	/// \param import_index ImportIndex of the node, if any.
	/// \returns Reference to node after it has been stored.
	auto add_node(Type node, ImportIndex const import_index = ImportIndex::eNone) -> Type& {
		auto const id = TreeNodeId{++m_prev_id};
		auto& base = static_cast<TreeNode&>(node);
		base.m_id = id;
		base.m_import_index = import_index;
		if (import_index > ImportIndex::eNone) { m_index_to_node_id.insert_or_assign(import_index, id); }
		auto const [it, _] = m_nodes.insert_or_assign(id, std::move(node));
		m_root_nodes.push_back(id);
		return it->second;
	}

	[[nodiscard]] auto get_node(this auto&& self, Id const id) {
		auto const it = self.m_nodes.find(id);
		return it != self.m_nodes.end() ? &it->second : nullptr;
	}

	[[nodiscard]] auto find_node_by_name(this auto&& self, std::string_view const name) {
		auto it = self.m_nodes.end();
		for (auto i = self.m_nodes.begin(); i != self.m_nodes.end(); ++i) {
			if (i->second.name == name) {
				it = i;
				break;
			}
		}
		return it != self.m_nodes.end() ? &it->second : nullptr;
	}

	[[nodiscard]] auto find_node_by_index(this auto&& self, ImportIndex const import_index) {
		auto it = self.m_nodes.end();
		if (auto const i = self.m_index_to_node_id.find(import_index); i != self.m_index_to_node_id.end()) { it = self.m_nodes.find(i->second); }
		return it != self.m_nodes.end() ? &it->second : nullptr;
	}

	/// \brief Establish a hierarchy between nodes.
	/// \param node Node to parent.
	/// \param parent_id Id of parent node.
	///
	/// After parenting, node will no longer be in the list of root nodes.
	/// No action is taken if the parent node doesn't exist.
	void set_parent(TreeNode& node, Id parent_id) {
		if (node.m_id == Id::eNone) { return; }

		auto* parent_node = get_node(parent_id);
		if (parent_node == nullptr) { return; }

		unparent(node);

		parent_node->m_children.push_back(node.m_id);
		node.m_parent_id = parent_id;
		std::erase_if(m_root_nodes, [id = node.m_id](Id const i) { return i == id; });
	}

	/// \brief Break hierarchy and establish as a root node.
	/// \param node Node to unparent.
	///
	/// After unparenting, node will be in the list of root nodes.
	/// No action is taken if node doesn't have a parent.
	void unparent(TreeNode& node) {
		if (node.m_parent_id == Id::eNone) { return; }

		auto* parent = get_node(std::exchange(node.m_parent_id, Id::eNone));
		if (parent == nullptr) { return; }

		std::erase_if(parent->m_children, [self = node.m_id](Id const id) { return self == id; });
		node.m_parent_id = Id::eNone;
		m_root_nodes.push_back(node.m_id);
	}

	/// \brief Compute the global transform matrix of a node.
	/// \param node Node to get the transform for.
	/// \returns Global transform of node.
	// NOLINTNEXTLINE(misc-no-recursion)
	[[nodiscard]] auto get_model_matrix(TreeNode const& node) const -> glm::mat4 {
		auto const ret = node.transform.get_matrix();
		if (auto const* parent = get_node(node.m_parent_id)) { return get_model_matrix(*parent) * ret; }
		return ret;
	}

	void remove_node(TreeNodeId const id) {
		auto const it = m_nodes.find(id);
		if (it == m_nodes.end()) { return; }
		remove_node_impl(it);
	}

	template <typename PredT>
	void remove_if(PredT pred) {
		for (auto it = m_nodes.begin(); it != m_nodes.end();) {
			auto& node = it->second;
			if (pred(node)) {
				it = remove_node_impl(it);
			} else {
				++it;
			}
		}
	}

	[[nodiscard]] auto get_root_ids() const -> std::span<TreeNodeId const> { return m_root_nodes; }
	[[nodiscard]] auto get_all_nodes() const -> std::unordered_map<TreeNodeId, Type> const& { return m_nodes; }

	/// \brief Append nodes to passed buffer.
	/// \param out Vector to append to.
	void fill_nodes(this auto&& self, NodeCache& out) {
		out.reserve(out.size() + self.m_nodes.size());
		for (auto& [_, node] : self.m_nodes) { out.push_back(&node); }
	}

	void clear_nodes() {
		m_nodes.clear();
		m_index_to_node_id.clear();
		m_root_nodes.clear();
	}

  private:
	template <typename It>
	auto remove_node_impl(It const it) -> It {
		auto const& node = it->second;
		m_index_to_node_id.erase(node.get_import_index());
		std::erase_if(m_root_nodes, [id = node.get_id()](TreeNodeId const i) { return i == id; });
		return m_nodes.erase(it);
	}

	std::unordered_map<TreeNodeId, Type> m_nodes{};
	std::unordered_map<ImportIndex, TreeNodeId> m_index_to_node_id{};
	std::vector<TreeNodeId> m_root_nodes{};
	int m_prev_id{};
};

/// \brief Tree visit wrapper that does nothing.
struct TreeVisitNullWrap {
	static constexpr auto begin_visit(TreeNode const& /*node*/) -> bool { return true; }
	static constexpr void end_visit(TreeNode const& /*node*/) {}
};

/// \brief Traverse the nodes of a tree with a wrapper and visitors.
/// \param wrap Wrapper that's called before and after a node's visit.
/// \param tree Tree that nodes belong to.
/// \param id Id of node to start visiting from.
/// \param visitors Visitors for each node.
///
/// Traverses the tree from the paseed node to its leaf nodes.
/// If wrap.begin_visit(node) returns false, the node is not visited.
/// visitor(node) is called for every visited node.
template <typename TreeT, typename WrapT, typename... VisitorT>
void tree_visit_wrap(WrapT&& wrap, TreeT&& tree, TreeNodeId const id, VisitorT&&... visitors) {
	auto* node = tree.get_node(id);
	if (node == nullptr) { return; }
	if (wrap.begin_visit(*node)) {
		(visitors(*node), ...);
		for (auto const child_id : node->get_children_ids()) { tree_visit_wrap(wrap, tree, child_id, visitors...); }
		wrap.end_visit(*node);
	}
}
/// \brief Traverse the nodes of a tree.
/// \param tree Tree that nodes belong to.
/// \param id Id of node to start visiting from.
/// \param visitors Visitors for each node.
///
/// Traverses the tree from the paseed node to its leaf nodes.
/// visitor(node) is called for every visited node.
template <typename TreeT, typename... VisitorT>
void tree_visit(TreeT&& tree, TreeNodeId const id, VisitorT&&... visitors) {
	tree_visit_wrap(TreeVisitNullWrap{}, tree, id, visitors...);
}

/// \brief Traverse the nodes of a tree with a wrapper and visitors.
/// \param wrap Wrapper that's called before and after a node's visit.
/// \param tree Tree to visit the nodes of.
/// \param visitors Visitors for each node.
///
/// Traverses the tree from its root nodes to all their leaf nodes.
/// If wrap.begin_visit(node) returns false, the node is not visited.
/// visitor(node) is called for every visited node.
template <typename WrapT, typename TreeT, typename... VisitorT>
void tree_visit_wrap(WrapT&& wrap, TreeT&& tree, VisitorT&&... visitors) {
	for (auto const id : tree.get_root_ids()) { tree_visit_wrap(wrap, tree, id, visitors...); }
}
/// \brief Traverse the nodes of a tree.
/// \param tree Tree to visit the nodes of.
/// \param visitors Visitors for each node.
///
/// Traverses the tree from its root nodes to all their leaf nodes.
/// visitor(node) is called for every visited node.
template <typename TreeT, typename... VisitorT>
void tree_visit(TreeT&& tree, VisitorT&&... visitors) {
	tree_visit_wrap(TreeVisitNullWrap{}, tree, visitors...);
}
} // namespace levk
