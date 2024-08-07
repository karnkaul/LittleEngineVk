#pragma once
#include <djson/json.hpp>
#include <levk/asset.hpp>
#include <levk/imported_scene.hpp>

namespace levk {
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
