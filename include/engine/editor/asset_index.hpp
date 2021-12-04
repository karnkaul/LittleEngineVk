#pragma once
#include <engine/assets/asset_store.hpp>
#include <engine/editor/types.hpp>

namespace le::edi {
class AssetIndex {
  public:
	using Select = TreeSelect::Select;

	static Select list(Span<std::size_t const> types, std::string_view filter = {}, std::string_view selectedURI = {});

	template <typename... Types>
	static Select list(std::string_view filter = {}, std::string_view selectedURI = {});
};

// impl

template <typename... Types>
AssetIndex::Select AssetIndex::list(std::string_view filter, std::string_view selectedURI) {
	if constexpr (sizeof...(Types) > 0) {
		static auto const types = AssetStore::typeHash<Types...>();
		return list(types, filter, selectedURI);
	} else {
		return list({}, filter, selectedURI);
	}
}
} // namespace le::edi
