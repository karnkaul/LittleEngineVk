#pragma once
#include <djson/json.hpp>
#include <spaced/node/node_tree.hpp>

namespace spaced {
struct NodeTree::Serializer {
	static auto serialize(dj::Json& out, NodeTree const& tree) -> void;
	static auto deserialize(dj::Json const& json, NodeTree& out) -> void;
};

namespace io {
auto to_json(dj::Json& out, glm::mat4 const& mat) -> void;
auto to_json(dj::Json& out, Transform const& transform) -> void;
auto from_json(dj::Json const& json, Transform& out) -> void;
auto from_json(dj::Json const& json, glm::mat4& out) -> void;
} // namespace io
} // namespace spaced
