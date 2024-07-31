#include <asset_tree.hpp>
#include <levk/imcpp/im_text.hpp>
#include <filesystem>

namespace {
namespace fs = std::filesystem;
void draw_tree(levk::IAssetStore const& store, fs::path const& base, std::span<levk::PathTree const> trees, bool const is_uri_tree) {
	for (auto const& tree : trees) {
		auto const path = base / tree.identifier;
		if (!tree.children.empty()) {
			if (ImGui::TreeNode(tree.identifier.c_str())) {
				draw_tree(store, path, tree.children, is_uri_tree);
				ImGui::TreePop();
			}
		} else {
			auto const uri = is_uri_tree ? path.generic_string() : tree.identifier;
			auto const* asset = store.get<levk::IAsset>(uri);
			if (asset == nullptr) { continue; }
			auto suffix = levk::FixedString{};
			if (is_uri_tree) { suffix = levk::FixedString{" [{}]", asset->get_type_name()}; }
			levk::im_text("{}{}", tree.identifier, suffix.as_view());
		}
	}
}
} // namespace

void AssetTree::draw(levk::IAssetStore const& asset_store, levk::Seconds const dt) {
	ImGui::SetNextItemWidth(100.0f);
	if (ImGui::BeginCombo("Group by", group_by.c_str())) {
		for (auto const& option : group_by_v) {
			if (ImGui::Selectable(option.c_str(), option.as_view() == group_by.as_view())) { group_by = option; }
		}
		ImGui::EndCombo();
	}

	static constexpr int flags_v = ImGuiWindowFlags_HorizontalScrollbar;
	ImGui::BeginChild("AssetTree", {-1.0f, -1.0f}, true, flags_v);
	update_trees(asset_store, dt);
	if (group_by.as_view() == group_by_v[0].as_view()) {
		draw_tree(asset_store, {}, uri_tree.children, true);
	} else {
		draw_tree(asset_store, {}, type_tree.children, false);
	}
	ImGui::EndChild();
}

void AssetTree::update_trees(levk::IAssetStore const& asset_store, levk::Seconds const dt) {
	if (auto const sentinel = asset_store.get_sentinel(); sentinel != current_sentinel) {
		uri_tree = {};
		type_tree = {};
		current_sentinel = sentinel;
		refresh_remain = refresh_delay_v;
		return;
	}

	if (cached_sentinel != current_sentinel) {
		if (refresh_remain > 0s) {
			refresh_remain -= dt;
			return;
		}

		refresh_trees(asset_store);
		cached_sentinel = current_sentinel;
	}
}

void AssetTree::refresh_trees(levk::IAssetStore const& asset_store) {
	auto uris = std::vector<std::string>{};
	asset_store.fill_uris(uris, &current_sentinel);

	uri_tree = levk::PathTree{};
	auto type_to_uris = std::unordered_map<std::string_view, std::vector<std::string>>{};
	for (auto& uri : uris) {
		auto const* asset = asset_store.get<levk::IAsset>(uri);
		if (asset == nullptr) { continue; }
		uri_tree.add(uri);
		type_to_uris[asset->get_type_name()].push_back(std::move(uri));
	}

	type_tree = {};
	for (auto& [type, uris] : type_to_uris) {
		auto& type_list = type_tree.children.emplace_back();
		type_list.identifier = std::string{type};
		type_list.children.reserve(uris.size());
		for (auto& uri : uris) { type_list.children.push_back({.identifier = std::move(uri)}); }
	}

	auto const sort_tree = [](levk::PathTree& out) {
		std::ranges::sort(out.children, [](levk::PathTree const& a, levk::PathTree const& b) { return a.identifier < b.identifier; });
	};
	uri_tree.visit(sort_tree);
	type_tree.visit(sort_tree);
}
