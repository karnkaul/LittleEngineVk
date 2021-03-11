// KT header-only library
// Requirements: C++17

#pragma once
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include "erased_semantics.hpp"

namespace kt {
///
/// \brief Fixed-size type erased storage
///
template <std::size_t N = sizeof(void*)>
class fixed_any_t final {
	template <typename T>
	static constexpr bool is_different_v = !std::is_same_v<std::decay_t<T>, fixed_any_t<N>>;
	template <typename T>
	using enable_if_different = std::enable_if_t<is_different_v<T>>;

  public:
#if defined(KT_FIXED_ANY_THROW)
	static constexpr bool throw_exception = true;
#else
	static constexpr bool throw_exception = false;
#endif

	constexpr fixed_any_t() noexcept = default;

	constexpr fixed_any_t(fixed_any_t&& rhs) noexcept;
	constexpr fixed_any_t(fixed_any_t const& rhs);
	constexpr fixed_any_t& operator=(fixed_any_t&& rhs) noexcept;
	constexpr fixed_any_t& operator=(fixed_any_t const& rhs);
	~fixed_any_t();

	///
	/// \brief Construct with object of type T
	///
	template <typename T, typename = enable_if_different<T>>
	constexpr fixed_any_t(T&& t);
	///
	/// \brief Assign to object of type T
	///
	template <typename T, typename = enable_if_different<T>>
	constexpr fixed_any_t& operator=(T&& t);
	///
	/// \brief Check if held type (if any) matches T
	///
	template <typename T>
	constexpr bool contains() const noexcept;
	///
	/// \brief Check if no type is held
	///
	constexpr bool empty() const noexcept;
	///
	/// \brief Obtain reference to T
	/// Throws / returns static reference on type mismatch
	///
	template <typename T>
	constexpr T const& get() const;
	///
	/// \brief Obtain reference to T
	/// Throws / returns static reference on type mismatch
	///
	template <typename T>
	constexpr T& get();
	///
	/// \brief Obtain a copy of T if contained, else fallback
	/// Throws / returns static reference on type mismatch
	///
	template <typename T>
	constexpr T value_or(T const& fallback) const;

	///
	/// \brief Destroy held type (if any)
	///
	constexpr bool clear() noexcept;

  private:
	template <typename T>
	constexpr void construct(T&& t);

	template <typename T, typename = std::enable_if_t<!std::is_lvalue_reference_v<T>>>
	constexpr void emplace(T&& t);

	template <typename T>
	constexpr void emplace(T const& t);

	template <typename T>
	static detail::erased_semantics_t const& erased() noexcept;

	alignas(std::max_align_t) std::aligned_storage_t<N, alignof(std::max_align_t)> m_bytes;
	detail::erased_semantics_t const* m_erased = nullptr;
};

template <std::size_t N>
constexpr fixed_any_t<N>::fixed_any_t(fixed_any_t&& rhs) noexcept : m_erased(rhs.m_erased) {
	m_erased->move_construct(&rhs.m_bytes, &m_bytes);
}

template <std::size_t N>
constexpr fixed_any_t<N>::fixed_any_t(fixed_any_t const& rhs) : m_erased(rhs.m_erased) {
	m_erased->copy_construct(&rhs.m_bytes, &m_bytes);
}

template <std::size_t N>
constexpr fixed_any_t<N>& fixed_any_t<N>::operator=(fixed_any_t&& rhs) noexcept {
	if (&rhs != this) {
		if (m_erased == rhs.m_erased) {
			if (m_erased) {
				m_erased->move_assign(&rhs.m_bytes, &m_bytes);
			}
		} else {
			clear();
			m_erased = rhs.m_erased;
			if (m_erased) {
				m_erased->move_construct(&rhs.m_bytes, &m_bytes);
			}
		}
	}
	return *this;
}

template <std::size_t N>
constexpr fixed_any_t<N>& fixed_any_t<N>::operator=(fixed_any_t const& rhs) {
	if (&rhs != this) {
		if (m_erased == rhs.m_erased) {
			if (m_erased) {
				m_erased->copy_assign(&rhs.m_bytes, &m_bytes);
			}
		} else {
			clear();
			m_erased = rhs.m_erased;
			if (m_erased) {
				m_erased->copy_construct(&rhs.m_bytes, &m_bytes);
			}
		}
	}
	return *this;
}

template <std::size_t N>
fixed_any_t<N>::~fixed_any_t() {
	clear();
}

template <std::size_t N>
template <typename T, typename>
constexpr fixed_any_t<N>::fixed_any_t(T&& t) {
	construct<T>(std::forward<T>(t));
}

template <std::size_t N>
template <typename T, typename>
constexpr fixed_any_t<N>& fixed_any_t<N>::operator=(T&& t) {
	construct<T>(std::forward<T>(t));
	return *this;
}

template <std::size_t N>
template <typename T>
constexpr bool fixed_any_t<N>::contains() const noexcept {
	return &erased<T>() == m_erased;
}

template <std::size_t N>
constexpr bool fixed_any_t<N>::empty() const noexcept {
	return m_erased == nullptr;
}

template <std::size_t N>
template <typename T>
constexpr T const& fixed_any_t<N>::get() const {
	if (contains<T>()) {
		return *std::launder(reinterpret_cast<T const*>(&m_bytes));
	}
	if constexpr (throw_exception) {
		throw std::runtime_error("fixed_any_t: Type mismatch!");
	} else {
		assert(false && "fixed_any: Type mismatch");
		std::terminate();
	}
}

template <std::size_t N>
template <typename T>
constexpr T& fixed_any_t<N>::get() {
	if (contains<T>()) {
		return *std::launder(reinterpret_cast<T*>(&m_bytes));
	}
	if constexpr (throw_exception) {
		throw std::runtime_error("fixed_any_t: Type mismatch!");
	} else {
		assert(false && "fixed_any: Type mismatch");
		std::terminate();
	}
}

template <std::size_t N>
template <typename T>
constexpr T fixed_any_t<N>::value_or(T const& fallback) const {
	if (contains<T>()) {
		return *std::launder(reinterpret_cast<T const*>(&m_bytes));
	}
	return fallback;
}

template <std::size_t N>
constexpr bool fixed_any_t<N>::clear() noexcept {
	if (m_erased) {
		m_erased->destroy(&m_bytes);
		m_erased = nullptr;
		return true;
	}
	return false;
}

template <std::size_t N>
template <typename T>
constexpr void fixed_any_t<N>::construct(T&& t) {
	if constexpr (std::is_same_v<T, std::nullptr_t>) {
		clear();
	} else {
		static_assert(is_different_v<T>, "fixed_any_t: Recursive storage is forbidden");
		static_assert(sizeof(T) <= N, "fixed_any_t: T is too large (compared to N)");
		emplace(std::forward<T>(t));
	}
}

template <std::size_t N>
template <typename T, typename>
constexpr void fixed_any_t<N>::emplace(T&& t) {
	auto const& e = erased<std::decay_t<T>>();
	if (m_erased && m_erased == &e) {
		m_erased->move_assign(std::addressof(t), &m_bytes);
	} else {
		clear();
		m_erased = &e;
		m_erased->move_construct(std::addressof(t), &m_bytes);
	}
}

template <std::size_t N>
template <typename T>
constexpr void fixed_any_t<N>::emplace(T const& t) {
	auto const& e = erased<std::decay_t<T>>();
	if (m_erased && m_erased == &e) {
		m_erased->copy_assign(std::addressof(t), &m_bytes);
	} else {
		clear();
		m_erased = &e;
		m_erased->copy_construct(std::addressof(t), &m_bytes);
	}
}

template <std::size_t N>
template <typename T>
detail::erased_semantics_t const& fixed_any_t<N>::erased() noexcept {
	static constexpr detail::erased_semantics_t const s_erased{detail::erased_semantics_t::tag_t<T>()};
	return s_erased;
}
} // namespace kt
