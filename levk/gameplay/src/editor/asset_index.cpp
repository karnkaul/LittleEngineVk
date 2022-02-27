#include <levk/gameplay/editor/asset_index.hpp>

namespace le::editor {
#define MU [[maybe_unused]]

AssetIndex::Select AssetIndex::list(MU AssetStore const& store, MU Span<Sign const> types, MU std::string_view filter, MU std::string_view selectedURI) {
	Select ret;
#if defined(LEVK_USE_IMGUI)
	auto index = store.index(types, filter);
	std::vector<TreeSelect::Group> groups;
	groups.reserve(index.maps.size());
	for (auto& map : index.maps) { groups.push_back({map.type.name, std::move(map.uris)}); }
	if (auto select = TreeSelect::list(groups, selectedURI)) { ret = select; }
#endif
	return ret;
}
} // namespace le::editor
