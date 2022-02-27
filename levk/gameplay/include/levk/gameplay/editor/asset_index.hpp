#pragma once
#include <dens/detail/sign.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/gameplay/editor/types.hpp>

namespace le::editor {
class AssetIndex {
  public:
	using Select = TreeSelect::Select;
	using Sign = AssetStore::Sign;

	static Select list(AssetStore const& store, Span<Sign const> types, std::string_view filter = {}, std::string_view selectedURI = {});

	template <typename... Types>
	static Select list(AssetStore const& store, std::string_view filter = {}, std::string_view selectedURI = {});
};

// impl

template <typename... Types>
AssetIndex::Select AssetIndex::list(AssetStore const& store, std::string_view filter, std::string_view selectedURI) {
	return list(store, store.signs<Types...>(), filter, selectedURI);
}
} // namespace le::editor
