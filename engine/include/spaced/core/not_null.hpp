#pragma once
#include <spaced/core/ptr.hpp>
#include <cassert>
#include <concepts>
#include <utility>

namespace spaced {
template <typename Type>
concept PointerLike = requires(Type const& t) {
	!std::same_as<Type, std::nullptr_t>;
	t != nullptr;
};

template <PointerLike Type>
class NotNull {
  public:
	NotNull(std::nullptr_t) = delete;
	auto operator=(std::nullptr_t) -> NotNull& = delete;

	template <std::convertible_to<Type> T>
	constexpr NotNull(T&& u) : m_t(std::forward<T>(u)) {
		assert(m_t != nullptr);
	}

	constexpr NotNull(Type t) : m_t(std::move(t)) { assert(m_t != nullptr); }

	template <std::convertible_to<Type> T>
	constexpr NotNull(NotNull<T> const& rhs) : m_t(rhs.get()) {}

	constexpr auto get() const -> Type const& { return m_t; }
	constexpr operator Type() const { return get(); }

	constexpr auto operator*() const -> decltype(auto) { return *get(); }
	constexpr auto operator->() const -> decltype(auto) { return get(); }

  private:
	Type m_t{};
};
} // namespace spaced
