#pragma once
#include <levk/core/enum_t.hpp>
#include <bitset>

namespace levk {
/// \brief Enum-indexed wrapper around std::bitset.
template <EnumT E, std::size_t Size = std::size_t(E::eCOUNT_)>
class EnumFlags : public std::bitset<Size> {
  public:
	using std::bitset<Size>::bitset;

	/// \brief Constructor taking multiple flags.
	/// \param to_set flags to set.
	/// \pre e must be non-negative and less than Size.
	template <std::same_as<E>... T>
	/*implicit*/ EnumFlags(T const... to_set) {
		(this->set(to_set), ...);
	}

	/// \brief Test if a flag is set.
	/// \param e Flag to test for.
	/// \returns true if set.
	/// \pre e must be non-negative and less than Size.
	[[nodiscard]] auto test(E const e) const -> bool { return std::bitset<Size>::test(static_cast<std::size_t>(e)); }
	/// \brief Test if any of given flags is set.
	/// \param flags Flags to test for.
	/// \returns true if any bit in flags is set.
	[[nodiscard]] auto test_any(EnumFlags const& flags) const -> bool { return (flags & *this).any(); }
	/// \brief Test if all of given flags are set.
	/// \param flags Flags to test for.
	/// \returns true if all bits in flags are set.
	[[nodiscard]] auto test_all(EnumFlags const& flags) const -> bool { return (flags & *this) == flags; }
	/// \brief Set a flag.
	/// \param e Flag to set.
	/// \pre e must be non-negative and less than Size.
	void set(E const e) { std::bitset<Size>::set(static_cast<std::size_t>(e)); }
	/// \brief Reset a flag.
	/// \param e Flag to reset.
	/// \pre e must be non-negative and less than Size.
	void reset(E const e) { std::bitset<Size>::reset(static_cast<std::size_t>(e)); }

	[[nodiscard]] auto operator[](E const e) -> decltype(auto) { return std::bitset<Size>::operator[](static_cast<std::size_t>(e)); }
	[[nodiscard]] auto operator[](E const e) const -> decltype(auto) { return std::bitset<Size>::operator[](static_cast<std::size_t>(e)); }
};
} // namespace levk
