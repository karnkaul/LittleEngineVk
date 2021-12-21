#pragma once
#include <type_traits>
#include <utility>

namespace le {
template <typename T>
concept comparable = requires(T const& a, T const& b) {
	{ a == b } -> std::same_as<bool>;
};

template <comparable T, typename Del = void>
class Unique {
  public:
	constexpr Unique(T t = T{}) noexcept : m_t(std::move(t)) {}

	constexpr Unique(Unique&& rhs) noexcept : Unique() { swap(*this, rhs); }
	constexpr Unique& operator=(Unique rhs) noexcept { return (swap(*this, rhs), *this); }
	constexpr ~Unique() noexcept;

	constexpr bool active() const noexcept { return m_t != T{}; }
	constexpr explicit operator bool() const noexcept { return active(); }

	constexpr T const& get() const noexcept { return m_t; }
	constexpr T& get() noexcept { return m_t; }
	constexpr T const& operator*() const noexcept { return get(); }
	constexpr T& operator*() noexcept { return get(); }
	constexpr T const* operator->() const noexcept { return &get(); }
	constexpr T* operator->() noexcept { return &get(); }

	friend constexpr void swap(Unique& lhs, Unique& rhs) noexcept {
		using std::swap;
		swap(lhs.m_t, rhs.m_t);
	}

  private:
	T m_t;
};

// impl
template <comparable T, typename Del>
constexpr Unique<T, Del>::~Unique() noexcept {
	if constexpr (!std::is_same_v<Del, void>) {
		if (this->m_t != T{}) { Del{}(this->m_t); }
	}
}
} // namespace le
