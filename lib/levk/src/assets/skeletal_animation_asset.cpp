#include <djson/json.hpp>
#include <levk/asset_store.hpp>
#include <levk/assets/bin_animation_channels.hpp>
#include <levk/assets/skeletal_animation_asset.hpp>

namespace levk {
auto SkeletalAnimationAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v) { return false; }

	auto const bytes = store.get_vfs().load_bytes(json["channels"].as_string());
	if (bytes.empty()) { return false; }

	auto channels = std::vector<AnimationChannel>{};
	from_bytes(bytes, channels);

	if (channels.empty()) { return false; }

	animation = {};
	animation.name = json["name"].as_string();
	for (auto& channel : channels) { animation.add_channel(std::move(channel)); }

	return true;
}
} // namespace levk
