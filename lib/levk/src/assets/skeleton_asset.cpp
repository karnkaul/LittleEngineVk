#include <djson/json.hpp>
#include <levk/asset_store.hpp>
#include <levk/assets/bin_transform_sampler.hpp>
#include <levk/assets/skeletal_animation_asset.hpp>
#include <levk/assets/skeleton_asset.hpp>
#include <levk/io/tree_io.hpp>

namespace levk {
auto SkeletonAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v) { return false; }

	auto joint_tree = JsonToTree{}.import_tree(json["joint_tree"]);
	if (joint_tree.get_all_nodes().empty()) { return false; }

	auto joint_ids = std::vector<TreeNodeId>{};
	for (auto const& import_index : json["joints_import_indices"].array_view()) {
		auto const* node = joint_tree.find_node_by_index(to_import_index(import_index.as<std::int64_t>()));
		if (node == nullptr) { return false; }
		joint_ids.push_back(node->get_id());
	}

	auto inverse_bind_matrices = std::vector<glm::mat4>{};
	if (auto const& in_mats = json["inverse_bind_matrices"]) {
		for (auto const& in_mat : in_mats.array_view()) {
			auto out_mat = glm::mat4{};
			from_json(in_mat, out_mat);
			inverse_bind_matrices.push_back(out_mat);
		}
		if (inverse_bind_matrices.size() != joint_ids.size()) { return false; }
	}

	auto root_joint = TreeNodeId::eNone;
	if (auto const& in_joint = json["root_joint"]["import_index"]) {
		auto const* root_node = joint_tree.find_node_by_index(to_import_index(in_joint.as<std::int64_t>()));
		if (root_node == nullptr) { return false; }
		root_joint = root_node->get_id();
	}

	if (inverse_bind_matrices.empty()) { inverse_bind_matrices.resize(joint_ids.size(), identity_mat_v); }

	m_animations.clear();
	m_animations.reserve(json["animations"].array_view().size());
	for (auto const& in_animation : json["animations"].array_view()) {
		auto const load_info = LoadInfo{
			.uri = in_animation.as_string(),
			.reload = info.reload,
		};
		auto const* asset = store.load<SkeletalAnimationAsset>(load_info);
		if (asset == nullptr) { continue; }
		m_animations.emplace_back(&asset->animation);
	}

	m_name = json["name"].as_string();
	m_joint_tree = std::move(joint_tree);
	m_joint_ids = std::move(joint_ids);
	m_inverse_bind_matrices = std::move(inverse_bind_matrices);
	m_root_joint = root_joint;

	auto create_info = Skeleton::CreateInfo{
		.joint_tree = m_joint_tree,
		.inverse_bind_matrices = m_inverse_bind_matrices,
		.joint_ids = m_joint_ids,
		.animations = m_animations,
	};
	m_skeleton = Skeleton{std::move(create_info)};
	m_skeleton.name = m_name;

	return true;
}
} // namespace levk
