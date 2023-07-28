#pragma once
#include <spaced/core/id.hpp>
#include <spaced/core/transform.hpp>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace spaced {
class Entity;

struct NodeCreateInfo;

class Node {
  public:
	using CreateInfo = NodeCreateInfo;
	class Tree;
	class Locator;

	auto id() const -> Id<Node> { return m_id; }
	auto parent() const -> std::optional<Id<Node>> { return m_parent; }
	auto children() const -> std::span<Id<Node> const> { return m_children; }

	std::optional<Id<Entity>> entity_id{};
	Transform transform{};
	std::string name{};

  private:
	Id<Node> m_id{0};
	std::optional<Id<Node>> m_parent{};
	std::vector<Id<Node>> m_children{};

	friend class NodeTree;
};

struct NodeCreateInfo {
	Transform transform{};
	std::string name{};
	std::optional<Id<Node>> parent{};
	std::optional<Id<Entity>> entity_id{};
};
} // namespace spaced
