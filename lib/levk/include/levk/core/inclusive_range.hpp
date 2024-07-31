#pragma once
#include <glm/common.hpp>

namespace levk {
/// \brief Ends of an inclusive range.
template <typename Type>
struct InclusiveRange {
	/// \brief Lower end of range.
	Type lo{};
	/// \brief Upper end of range.
	Type hi{};

	/// \brief Clamp input to stored range.
	/// \param in Input to clamp.
	/// \returns in clamped between lo and hi.
	constexpr auto clamp(Type const& in) const -> Type {
		using glm::clamp;
		return clamp(in, lo, hi);
	}

	template <typename T>
	constexpr operator InclusiveRange<T>() const {
		return {.lo = static_cast<T>(lo), .hi = static_cast<T>(hi)};
	}
};
} // namespace levk
