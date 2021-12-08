#pragma once
#include <dens/detail/sign.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/editor/types.hpp>

namespace le::edi {
class AssetIndex {
  public:
	using Select = TreeSelect::Select;
	using Sign = AssetStore::Sign;

	static Select list(Span<Sign const> types, std::string_view filter = {}, std::string_view selectedURI = {});

	template <typename... Types>
	static Select list(std::string_view filter = {}, std::string_view selectedURI = {});
};

// impl

template <typename... Types>
AssetIndex::Select AssetIndex::list(std::string_view filter, std::string_view selectedURI) {
	return list(AssetStore::signs<Types...>(), filter, selectedURI);
}
} // namespace le::edi
