#include <djson/json.hpp>
#include <le/node/node_tree_serializer.hpp>

namespace le {
namespace {
template <glm::length_t Dim, typename T = float>
auto glm_vec_from_json(dj::Json const& json, glm::vec<Dim, T> const& fallback = {}, std::size_t offset = 0) -> glm::vec<Dim, T> {
	auto ret = glm::vec<Dim, T>{};
	ret.x = json[offset + 0].as<T>(fallback.x);
	if constexpr (Dim > 1) { ret.y = json[offset + 1].as<T>(fallback.y); }
	if constexpr (Dim > 2) { ret.z = json[offset + 2].as<T>(fallback.z); }
	if constexpr (Dim > 3) { ret.w = json[offset + 3].as<T>(fallback.w); }
	return ret;
}

template <glm::length_t Dim>
void add_to(dj::Json& out, glm::vec<Dim, float> const& vec) {
	out.push_back(vec.x);
	if constexpr (Dim > 1) { out.push_back(vec.y); }
	if constexpr (Dim > 2) { out.push_back(vec.z); }
	if constexpr (Dim > 3) { out.push_back(vec.w); }
}
} // namespace

auto io::from_json(dj::Json const& json, glm::mat4& out) -> void {
	out[0] = glm_vec_from_json<4>(json, out[0]);
	out[1] = glm_vec_from_json<4>(json, out[1], 4);
	out[2] = glm_vec_from_json<4>(json, out[2], 8);
	out[3] = glm_vec_from_json<4>(json, out[3], 12);
}

auto io::from_json(dj::Json const& json, Transform& out) -> void {
	auto mat = glm::mat4{1.0f};
	from_json(json, mat);
	out.decompose(mat);
}

auto io::to_json(dj::Json& out, glm::mat4 const& mat) -> void {
	for (glm::length_t i = 0; i < 4; ++i) { add_to(out, mat[i]); }
}

auto io::to_json(dj::Json& out, Transform const& transform) -> void { to_json(out, transform.matrix()); }

auto NodeTree::Serializer::serialize(dj::Json& out, NodeTree const& tree) -> void {
	auto& out_nodes = out["nodes"];
	for (auto const& [id, in_node] : tree.m_nodes) {
		auto out_node = dj::Json{};
		out_node["name"] = in_node.name;
		io::to_json(out_node["transform"], in_node.transform);
		out_node["id"] = in_node.id().value();
		if (auto parent = in_node.parent()) { out_node["parent"] = parent.value().value(); }
		if (!in_node.children().empty()) {
			auto out_children = dj::Json{};
			for (auto id : in_node.children()) { out_children.push_back(id.value()); }
			out_node["children"] = std::move(out_children);
		}
		out_nodes.push_back(std::move(out_node));
	}
	auto& out_roots = out["roots"];
	for (auto const id : tree.m_roots) { out_roots.push_back(id.value()); }
	out["max_id"] = tree.m_next_id;
}

auto NodeTree::Serializer::deserialize(dj::Json const& json, NodeTree& out) -> void {
	for (auto const& in_node : json["nodes"].array_view()) {
		auto transform = Transform{};
		io::from_json(in_node["transform"], transform);
		auto nci = Node::CreateInfo{
			.transform = transform,
			.name = in_node["name"].as<std::string>(),
		};
		if (auto const& parent = in_node["parent"]) { nci.parent = parent.as<std::size_t>(); }
		auto children = std::vector<Id<Node>>{};
		for (auto const& in_child : in_node["children"].array_view()) { children.emplace_back(in_child.as<Id<Node>::id_type>()); }
		auto const id = in_node["id"].as<std::size_t>();
		out.m_nodes.insert_or_assign(id, make_node(id, std::move(children), std::move(nci)));
		out.m_next_id = std::max(out.m_next_id, id);
	}
	for (auto const& in_root : json["roots"].array_view()) { out.m_roots.emplace_back(in_root.as<std::size_t>()); }
	out.m_next_id = std::max(out.m_next_id, json["max_id"].as<Id<Node>::id_type>());
}
} // namespace le
