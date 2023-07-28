#pragma once
#include <spaced/core/ptr.hpp>
#include <spaced/node/node.hpp>
#include <optional>
#include <unordered_map>

namespace spaced {
class NodeTree {
  public:
	struct Data {
		std::optional<Id<Entity>> entity_id{};
		Transform transform{};
		std::string name{};
		Id<Node> id{0};
		std::optional<Id<Node>> parent{};
		std::vector<Id<Node>> children{};
	};

	using CreateInfo = NodeCreateInfo;
	using DataMap = std::unordered_map<Id<Node>::id_type, Data>;

	// Defined in node_tree_serializer
	struct Serializer;

	auto add(CreateInfo const& create_info) -> Node&;
	auto insert_or_assign(Id<Node> id, CreateInfo const& create_info) -> Node&;
	void remove(Id<Node> id);

	[[nodiscard]] auto find(Id<Node> id) const -> Ptr<Node const>;
	auto find(Id<Node> id) -> Ptr<Node>;

	[[nodiscard]] auto get(Id<Node> id) const -> Node const&;
	auto get(Id<Node> id) -> Node&;

	void reparent(Node& out, std::optional<Id<Node>> new_parent);
	[[nodiscard]] auto global_transform(Node const& node) const -> glm::mat4;
	[[nodiscard]] auto global_transform(Id<Node> id) const -> glm::mat4;
	[[nodiscard]] auto global_position(Node const& node) const -> glm::vec3;
	[[nodiscard]] auto global_position(Id<Node> id) const -> glm::vec3;

	[[nodiscard]] auto find_by_name(std::string_view name) const -> std::optional<Id<Node>>;

	void clear();

	[[nodiscard]] auto roots() const -> std::span<Id<Node> const> { return m_roots; }

	// Use with caution
	auto import_tree(DataMap nodes, std::vector<Id<Node>> roots) -> void;

	template <typename Func>
	void for_each(Func&& func) {
		for (auto& [_, node] : m_nodes) { func(node); }
	}

  private:
	using Map = std::unordered_map<Id<Node>::id_type, Node>;
	[[nodiscard]] static auto make_node(Id<Node> self, std::vector<Id<Node>> children, CreateInfo create_info = {}) -> Node;

	void remove_child_from_parent(Node& out);
	void destroy_children(Node& out);

	Map m_nodes{};
	std::vector<Id<Node>> m_roots{};
	Id<Node>::id_type m_next_id{};
};

///
/// \brief Enables updating nodes but not adding/removing nodes to/from the tree.
///
/// Animations / editors / etc use NodeLocators to affect nodes safely without
/// any possibility of adding/removing nodes to/from the tree.
///
class NodeLocator {
  public:
	NodeLocator(NodeTree& out_tree) : m_out(out_tree) {}

	[[nodiscard]] auto find(Id<Node> id) const -> Ptr<Node> { return m_out.find(id); }
	[[nodiscard]] auto get(Id<Node> id) const -> Node& { return m_out.get(id); }
	[[nodiscard]] auto find_by_name(std::string_view name) const -> std::optional<Id<Node>> { return m_out.find_by_name(name); }
	void reparent(Node& out, Id<Node> new_parent) const { return m_out.reparent(out, new_parent); }
	[[nodiscard]] auto global_transform(Node const& node) const -> glm::mat4 { return m_out.global_transform(node); }
	[[nodiscard]] auto global_position(Node const& node) const -> glm::vec3 { return m_out.global_position(node); }
	[[nodiscard]] auto global_position(Id<Node> id) const -> glm::vec3 { return m_out.global_position(id); }
	[[nodiscard]] auto roots() const -> std::span<Id<Node> const> { return m_out.roots(); }
	// [[nodiscard]] auto map() const -> NodeTree::Map const& { return m_out.map(); }

  private:
	// NOLINTNEXTLINE
	NodeTree& m_out;
};
} // namespace spaced
