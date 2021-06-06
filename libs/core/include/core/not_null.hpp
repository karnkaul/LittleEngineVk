#pragma once
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace le {
namespace detail {
template <typename T, typename = void>
struct is_comparable_to_nullptr : std::false_type {};
template <typename T>
struct is_comparable_to_nullptr<T, std::enable_if_t<std::is_convertible_v<decltype(std::declval<T>() != nullptr), bool>>> : std::true_type {};
template <typename T>
constexpr bool is_comparable_to_nullptr_v = is_comparable_to_nullptr<T>::value;
} // namespace detail

template <typename T>
class not_null {
	static_assert(detail::is_comparable_to_nullptr_v<T>, "Cannot compare T to nullptr");

  public:
	using type = T;

	constexpr not_null(type&& ptr) noexcept : m_ptr(std::move(ptr)) { assert(m_ptr != nullptr); }
	constexpr not_null(type const& ptr) noexcept : m_ptr(ptr) { assert(m_ptr != nullptr); }

	constexpr not_null(std::nullptr_t) = delete;
	constexpr not_null& operator=(std::nullptr_t) = delete;

	[[nodiscard]] constexpr operator type const &() const noexcept { return m_ptr; }
	[[nodiscard]] constexpr type const& get() const noexcept { return m_ptr; }
	constexpr decltype(auto) operator*() const noexcept { return *m_ptr; }
	constexpr decltype(auto) operator->() const noexcept { return m_ptr; }

  private:
	type m_ptr;
};

template <typename T>
constexpr bool operator==(not_null<T> const& lhs, not_null<T> const& rhs) noexcept {
	return lhs.get() == rhs.get();
}
} // namespace le
