#pragma once
#include <levk/asset.hpp>
#include <levk/mesh.hpp>

namespace dj {
class Json;
}

namespace levk {
class StaticPrimitiveAsset : public IAsset {
  public:
	using PrimitiveType = IStaticPrimitive;

	static constexpr std::string_view type_name_v{"StaticMeshPrimitiveAsset"};

	std::unique_ptr<IStaticPrimitive> primitive{};

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;
};

class SkinnedPrimitiveAsset : public IAsset {
  public:
	using PrimitiveType = ISkinnedPrimitive;

	static constexpr std::string_view type_name_v{"SkinnedMeshPrimitiveAsset"};

	std::unique_ptr<ISkinnedPrimitive> primitive{};

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;
};
} // namespace levk
