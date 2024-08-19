#pragma once
#include <levk/node_tree.hpp>
#include <levk/transform.hpp>
#include <optional>

namespace levk {
struct ImportedCamera {
	enum class Type { ePerspective, eOrthographic };

	std::string name{};
	Type type{};
	float z_near{};
	std::optional<float> z_far{};
	std::optional<Radians> y_fov{};
};

class ImportedNode : public TreeNode {
  public:
	std::string mesh{};
	std::string skeleton{};
	std::optional<ImportedCamera> camera{};

	[[nodiscard]] auto is_valid() const -> bool { return get_import_index() > ImportIndex::eNone; }
};

class ImportedScene : public BasicNodeTree<ImportedNode> {};
} // namespace levk
