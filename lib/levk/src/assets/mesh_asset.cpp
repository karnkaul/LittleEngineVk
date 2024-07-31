#include <levk/asset_store.hpp>
#include <levk/assets/bin_geometry.hpp>
#include <levk/assets/lit_material_asset.hpp>
#include <levk/assets/mesh_asset.hpp>
#include <levk/assets/primitive_asset.hpp>
#include <levk/assets/skeleton_asset.hpp>
#include <levk/assets/texture_asset.hpp>
#include <levk/io/json_io.hpp>

namespace levk {
namespace {
template <typename InT, typename OutT = InT::PrimitiveType>
auto load_primitives(IAssetStore& store, dj::Json const& json, bool const reload) -> std::vector<NotNull<OutT*>> {
	auto ret = std::vector<NotNull<OutT*>>{};
	for (auto const& in_primitive : json.array_view()) {
		auto const load_info = IAsset::LoadInfo{
			.uri = in_primitive.as_string(),
			.reload = reload,
		};
		auto const* asset = store.load<InT>(load_info);
		if (asset == nullptr) { continue; }
		ret.push_back(asset->primitive.get());
	}
	return ret;
}
} // namespace

auto StaticMeshAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v) { return false; }

	auto const primitives = load_primitives<StaticPrimitiveAsset>(store, json["primitives"], info.reload);
	if (primitives.empty()) { return false; }

	mesh = {};
	mesh.name = json["name"].as_string();
	mesh.primitives.reserve(primitives.size());
	for (auto const& primitive : primitives) { mesh.primitives.emplace_back(primitive); }

	return true;
}

auto SkinnedMeshAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v) { return false; }

	auto const* skeleton_asset = store.load<SkeletonAsset>(json["skeleton"].as_string());
	if (skeleton_asset == nullptr) { return false; }

	auto const primitives = load_primitives<SkinnedPrimitiveAsset>(store, json["primitives"], info.reload);
	if (primitives.empty()) { return false; }

	mesh = SkinnedMesh{skeleton_asset->get_skeleton()};
	mesh.name = json["name"].as_string();
	mesh.primitives.reserve(primitives.size());
	for (auto const& primitive : primitives) { mesh.primitives.emplace_back(primitive); }

	return true;
}
} // namespace levk
