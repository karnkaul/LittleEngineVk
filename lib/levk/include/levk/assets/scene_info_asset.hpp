#pragma once
#include <djson/json.hpp>
#include <levk/asset.hpp>
#include <levk/scene_info.hpp>

namespace levk {
class SceneInfoAsset : public IAsset {
  public:
	static constexpr std::string_view type_name_v{"SceneInfoAsset"};

	SceneInfo scene{};

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;
};
} // namespace levk
