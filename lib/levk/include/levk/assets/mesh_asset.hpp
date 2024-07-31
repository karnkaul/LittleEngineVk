#pragma once
#include <levk/asset.hpp>
#include <levk/mesh.hpp>

namespace dj {
class Json;
}

namespace levk {
class StaticMeshAsset : public IAsset {
  public:
	using MeshType = StaticMesh;

	static constexpr std::string_view type_name_v{"StaticMeshAsset"};

	StaticMesh mesh{};

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;
};

class SkinnedMeshAsset : public IAsset {
  public:
	using MeshType = SkinnedMesh;

	static constexpr std::string_view type_name_v{"SkinnedMeshAsset"};

	SkinnedMesh mesh{};

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;
};
} // namespace levk
