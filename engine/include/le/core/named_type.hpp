#pragma once
#include <string_view>

namespace le {
struct NamedType {
	NamedType() = default;
	NamedType(NamedType const&) = default;
	NamedType(NamedType&&) = default;
	auto operator=(NamedType const&) -> NamedType& = default;
	auto operator=(NamedType&&) -> NamedType& = default;

	virtual ~NamedType() = default;

	[[nodiscard]] virtual auto type_name() const -> std::string_view { return "unnamed"; }
};
} // namespace le
