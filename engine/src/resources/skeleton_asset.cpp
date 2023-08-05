#include <le/node/node_tree_serializer.hpp>
#include <le/resources/animation_asset.hpp>
#include <le/resources/resources.hpp>
#include <le/resources/skeleton_asset.hpp>

namespace le {
auto SkeletonAsset::try_load(Uri const& uri) -> bool {
	auto json = read_json(uri);
	if (!json) { return false; }

	skeleton = {};
	NodeTree::Serializer::deserialize(json["joint_tree"], skeleton.joint_tree);
	for (auto const& joint : json["ordered_joint_ids"].array_view()) { skeleton.ordered_joint_ids.emplace_back(joint.as<Id<Node>::id_type>()); }
	for (auto const& ibm : json["inverse_bind_matrices"].array_view()) { io::from_json(ibm, skeleton.inverse_bind_matrices.emplace_back()); }
	for (auto const& animation_uri : json["animations"].array_view()) {
		auto const* animation_asset = Resources::self().load<AnimationAsset>(animation_uri.as_string());
		if (animation_asset == nullptr) { continue; }
		skeleton.animations.push_back(&animation_asset->animation);
	}

	return true;
}
} // namespace le
