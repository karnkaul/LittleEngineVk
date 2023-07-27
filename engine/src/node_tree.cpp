#include <spaced/engine/core/logger.hpp>
#include <spaced/engine/error.hpp>
#include <spaced/engine/node_tree.hpp>
#include <algorithm>
#include <format>
#include <utility>

namespace spaced {
namespace {
auto const g_log{logger::Logger{"NodeTree"}};
}

auto NodeTree::insert_or_assign(Id<Node> id, CreateInfo const& create_info) -> Node& {
	auto node = Node{};
	node.m_id = id;
	m_next_id = std::max(m_next_id, id.value() + 1);
	node.transform = create_info.transform;
	node.entity_id = create_info.entity_id;
	assert(!create_info.parent || node.m_id != *create_info.parent);
	node.name = create_info.name.empty() ? "(Unnamed)" : create_info.name;
	if (create_info.parent) {
		if (auto it = m_nodes.find(*create_info.parent); it != m_nodes.end()) {
			it->second.m_children.push_back(node.m_id);
			node.m_parent = create_info.parent;
		} else {
			g_log.warn("Invalid parent Id<Node>: {}", create_info.parent->value());
		}
	}
	if (node.m_parent == 0u) { m_roots.push_back(node.m_id); }
	auto [it, _] = m_nodes.insert_or_assign(node.m_id, std::move(node));
	return it->second;
}

auto NodeTree::add(CreateInfo const& create_info) -> Node& { return insert_or_assign(m_next_id, create_info); }

void NodeTree::remove(Id<Node> id) {
	if (auto it = m_nodes.find(id); it != m_nodes.end()) {
		remove_child_from_parent(it->second);
		destroy_children(it->second);
		m_nodes.erase(it);
		std::erase(m_roots, id);
	}
}

void NodeTree::reparent(Node& out, std::optional<Id<Node>> new_parent) {
	assert(!new_parent || out.m_id != *new_parent);
	if ((!out.m_parent) && (new_parent)) { std::erase(m_roots, out.m_id); }
	if ((out.m_parent) && (!new_parent)) { m_roots.push_back(out.m_id); }
	remove_child_from_parent(out);
	if (new_parent) {
		if (auto* parent = find(*new_parent)) { parent->m_children.push_back(out.m_id); }
	}
	out.m_parent = new_parent;
}

// NOLINTNEXTLINE
auto NodeTree::global_transform(Node const& node) const -> glm::mat4 {
	auto ret = node.transform.matrix();
	if (!node.m_parent) { return ret; }
	assert(*node.m_parent != node.id());
	if (auto const* parent = find(*node.m_parent)) { return global_transform(*parent) * ret; }
	return ret;
}

auto NodeTree::global_transform(Id<Node> id) const -> glm::mat4 {
	auto const* node = find(id);
	if (node == nullptr) { return glm::identity<glm::mat4>(); }
	return global_transform(*node);
}

// NOLINTNEXTLINE
auto NodeTree::global_position(Node const& node) const -> glm::vec3 {
	auto ret = node.transform.position();
	if (!node.m_parent) { return ret; }
	if (auto const* parent = find(*node.m_parent)) { return global_position(*parent) + ret; }
	return ret;
}

auto NodeTree::global_position(Id<Node> id) const -> glm::vec3 {
	auto const* node = find(id);
	if (node == nullptr) { return {}; }
	return global_position(*node);
}

void NodeTree::clear() {
	m_nodes.clear();
	m_roots.clear();
}

auto NodeTree::find_by_name(std::string_view name) const -> std::optional<Id<Node>> {
	if (name.empty()) { return {}; }
	for (auto const& [id, node] : m_nodes) {
		if (node.name == name) { return id; }
	}
	return {};
}

// NOLINTNEXTLINE
auto NodeTree::import_tree(DataMap nodes, std::vector<Id<Node>> roots) -> void {
	m_nodes.clear();
	m_next_id = {};
	for (auto& [id, data] : nodes) {
		auto node = Node{};
		node.m_id = data.id;
		node.m_children = std::move(data.children);
		node.m_parent = data.parent;
		node.transform = data.transform;
		node.name = std::move(data.name);
		m_nodes.insert_or_assign(id, std::move(node));
		m_next_id = std::max(m_next_id, id + 1);
	}
	m_roots = std::move(roots);
}

auto NodeTree::make_node(Id<Node> self, std::vector<Id<Node>> children, CreateInfo create_info) -> Node {
	auto ret = Node{};
	ret.m_id = self;
	ret.m_parent = create_info.parent;
	ret.m_children = std::move(children);
	ret.entity_id = create_info.entity_id;
	ret.transform = create_info.transform;
	ret.name = std::move(create_info.name);
	return ret;
}

void NodeTree::remove_child_from_parent(Node& out) {
	if (!out.m_parent) { return; }
	if (auto* parent = find(*out.m_parent)) {
		std::erase(parent->m_children, out.m_id);
		out.m_parent = {};
	}
}

// NOLINTNEXTLINE
void NodeTree::destroy_children(Node& out) {
	for (auto const id : out.m_children) {
		if (auto it = m_nodes.find(id); it != m_nodes.end()) {
			destroy_children(it->second);
			m_nodes.erase(it);
		}
	}
	out.m_children.clear();
}

auto NodeTree::find(Id<Node> id) const -> Ptr<Node const> {
	if (auto it = m_nodes.find(id); it != m_nodes.end()) { return &it->second; }
	return {};
}

// NOLINTNEXTLINE
auto NodeTree::find(Id<Node> id) -> Ptr<Node> { return const_cast<Node*>(std::as_const(*this).find(id)); }

auto NodeTree::get(Id<Node> id) const -> Node const& {
	auto const* ret = find(id);
	if (ret == nullptr) { throw Error{std::format("Invalid entity id: {}", id.value())}; }
	return *ret;
}

// NOLINTNEXTLINE
auto NodeTree::get(Id<Node> id) -> Node& { return const_cast<Node&>(std::as_const(*this).get(id)); }
} // namespace spaced
