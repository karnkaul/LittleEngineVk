#pragma once
#include <levk/asset_store.hpp>
#include <levk/core/c_string.hpp>
#include <levk/path_tree.hpp>
#include <array>

struct AssetTree {
	static constexpr auto refresh_delay_v{150ms};

	static constexpr auto group_by_v = std::array{
		levk::CString{"URI"},
		levk::CString{"type"},
	};

	levk::IAssetStore::Sentinel cached_sentinel{};
	levk::IAssetStore::Sentinel current_sentinel{};
	levk::PathTree uri_tree{};
	levk::PathTree type_tree{};
	levk::CString group_by{group_by_v[0]};
	levk::Seconds refresh_remain{};

	void draw(levk::IAssetStore const& asset_store, levk::Seconds dt);

	void update_trees(levk::IAssetStore const& asset_store, levk::Seconds dt);
	void refresh_trees(levk::IAssetStore const& store);
};
