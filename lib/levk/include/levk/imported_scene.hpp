#pragma once
#include <levk/import_index.hpp>
#include <levk/transform.hpp>
#include <optional>
#include <string>
#include <vector>

namespace levk {
struct ImportedCamera {
	enum class Type { ePerspective, eOrthographic };

	Type type{};
	float z_near{};
	std::optional<float> z_far{};
	std::optional<Radians> y_fov{};
};

struct ImportedNode {
	ImportIndex index{ImportIndex::eNone};
	std::string name{};

	Transform transform{};
	ImportIndex parent{ImportIndex::eNone};
	std::vector<ImportIndex> children{};

	std::string mesh{};
	std::string skeleton{};
	std::optional<ImportedCamera> camera{};

	[[nodiscard]] auto is_valid() const -> bool { return index > ImportIndex::eNone; }
};

struct ImportedScene {
	std::vector<ImportedNode> nodes{};
	std::vector<ImportIndex> root_nodes{};

	[[nodiscard]] auto get_node(ImportIndex index) const -> ImportedNode const&;
};
} // namespace levk
