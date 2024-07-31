#pragma once
#include <levk/core/polymorphic.hpp>
#include <string_view>

namespace levk {
/// \brief Forward declaration of AssetStore interface.
class IAssetStore;

/// \brief Abstract base class for assets.
class IAsset : public Polymorphic {
  public:
	struct LoadInfo {
		std::string_view uri{};
		bool reload{};
	};

	/// \brief Get the name of this asset type.
	[[nodiscard]] virtual auto get_type_name() const -> std::string_view = 0;

	/// \brief Load the asset.
	/// \param store Reference to owning AssetStore.
	/// \param info LoadInfo for the asset.
	/// \returns true on success.
	virtual auto load(IAssetStore& store, LoadInfo const& info) -> bool = 0;
};

template <typename Type>
concept HasTypeNameT = requires() {
	{ Type::type_name_v } -> std::convertible_to<std::string_view>;
};

template <typename Type>
constexpr auto get_type_name() -> std::string_view {
	if constexpr (HasTypeNameT<Type>) {
		return Type::type_name_v;
	} else {
		return "unknown";
	}
}
} // namespace levk
