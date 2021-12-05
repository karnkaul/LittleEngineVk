#pragma once
#include <dens/detail/sign.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/editor/types.hpp>

namespace le::edi {
class AssetIndex {
  public:
	using Select = TreeSelect::Select;
	using Sign = dens::detail::sign_t;

	static Select list(Span<Sign const> types, std::string_view filter = {}, std::string_view selectedURI = {});

	template <typename... Types>
	static Select list(std::string_view filter = {}, std::string_view selectedURI = {});
};

// impl

template <typename... Types>
AssetIndex::Select AssetIndex::list(std::string_view filter, std::string_view selectedURI) {
	if constexpr (sizeof...(Types) > 0) {
		static auto const types = AssetStore::sign<Types...>();
		return list(types, filter, selectedURI);
	} else {
		return list({}, filter, selectedURI);
	}
}
} // namespace le::edi
