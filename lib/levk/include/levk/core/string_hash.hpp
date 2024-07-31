#pragma once
#include <concepts>
#include <string_view>

namespace levk {
/// \brief Concept for types string_view can be constructed from.
template <typename Type>
concept StringyT = std::constructible_from<std::string_view, Type>;

/// \brief Generalized string hasher.
struct StringHash {
	using is_transparent = void;

	template <StringyT Type>
	auto operator()(Type const& t) const -> size_t {
		return std::hash<std::string_view>{}(std::string_view{t});
	}
};
} // namespace levk
