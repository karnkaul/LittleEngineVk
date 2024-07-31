#include <levk/assets/bin_geometry.hpp>
#include <levk/io/binary_io.hpp>

void levk::to_bytes(std::vector<std::byte>& out, VertexArray const& vertex_array) {
	if (vertex_array.is_empty()) { return; }
	auto writer = binary::Writer{out};
	writer.write_next(std::span{vertex_array.vertices});
	writer.write_next(std::span{vertex_array.indices});
}

void levk::from_bytes(std::span<std::byte const> bytes, VertexArray& out) {
	auto ret = VertexArray{};
	auto reader = binary::Reader{bytes};
	if (!reader.read_next(ret.vertices)) { return; }
	if (reader.get_current_header() != nullptr && !reader.read_next(ret.indices)) { return; }
	out.append(ret.vertices, ret.indices);
}

void levk::to_bytes(std::vector<std::byte>& out, VertexSkin const& vertex_skin) {
	if (vertex_skin.joint_indices.empty() || vertex_skin.weights.empty()) { return; }
	auto writer = binary::Writer{out};
	writer.write_next(std::span{vertex_skin.joint_indices});
	writer.write_next(std::span{vertex_skin.weights});
}

void levk::from_bytes(std::span<std::byte const> bytes, VertexSkin& out) {
	auto ret = VertexSkin{};
	auto reader = binary::Reader{bytes};
	if (!reader.read_next(ret.joint_indices)) { return; }
	if (reader.get_current_header() != nullptr && !reader.read_next(ret.weights)) { return; }
	out.joint_indices.reserve(out.joint_indices.size() + ret.joint_indices.size());
	out.joint_indices.insert(out.joint_indices.end(), ret.joint_indices.begin(), ret.joint_indices.end());
	out.weights.reserve(out.weights.size() + ret.weights.size());
	out.weights.insert(out.weights.end(), ret.weights.begin(), ret.weights.end());
}
