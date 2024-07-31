#pragma once
#include <levk/core/ptr.hpp>
#include <cassert>
#include <concepts>
#include <utility>

namespace levk {
/// \brief Concept that models a native/smart pointer.
template <typename Type>
concept PointerLike = requires(Type const& t) {
	!std::same_as<Type, std::nullptr_t>;
	t != nullptr;
};

/// \brief Pointer that cannot be null.
template <PointerLike Type>
class NotNull {
  public:
	NotNull(std::nullptr_t) = delete;
	auto operator=(std::nullptr_t) -> NotNull& = delete;

	/// \brief Constructor from a convertible input.
	/// \param t Object that can be converted to Type.
	/// \pre t must not commpare equal to nullptr.
	template <std::convertible_to<Type> T>
	constexpr NotNull(T&& t) : m_t(std::forward<T>(t)) {
		assert(m_t != nullptr);
	}

	/// \brief Move and swap constructor.
	/// \param t Object of Type.
	/// \pre t must not compare equal to nullptr.
	constexpr NotNull(Type t) : m_t(std::move(t)) { assert(m_t != nullptr); }

	/// \brief Converting constructor.
	/// \param rhs NotNull object that is convertible to Type.
	template <std::convertible_to<Type> T>
	constexpr NotNull(NotNull<T> const& rhs) : m_t(rhs.get()) {}

	/// \brief Obtain the stored pointer.
	/// \returns Const reference to the underlying pointer.
	constexpr auto get() const -> Type const& { return m_t; }
	constexpr operator Type() const { return get(); }

	constexpr auto operator*() const -> decltype(auto) { return *get(); }
	constexpr auto operator->() const -> decltype(auto) { return get(); }

  private:
	Type m_t{};
};
} // namespace levk
