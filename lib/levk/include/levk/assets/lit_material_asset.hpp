#pragma once
#include <levk/asset.hpp>
#include <levk/materials/lit.hpp>
#include <optional>

namespace levk {
class LitMaterialAsset : public IAsset {
  public:
	static constexpr std::string_view type_name_v{"LitMaterialAsset"};

	std::optional<material::Lit> material{};

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;
};
} // namespace levk
