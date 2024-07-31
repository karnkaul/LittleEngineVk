#pragma once
#include <djson/json.hpp>
#include <levk/asset.hpp>
#include <levk/core/ptr.hpp>
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
	static constexpr std::string_view type_name_v{"ImportedNode"};

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

class ImportedSceneAsset : public IAsset {
  public:
	static constexpr std::string_view type_name_v{"ImportedSceneAsset"};

	[[nodiscard]] static auto to_node(dj::Json const& json) -> ImportedNode;

	ImportedScene imported_scene{};

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;
};
} // namespace levk
