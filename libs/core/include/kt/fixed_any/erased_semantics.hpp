// KT header-only library
// Requirements: C++17

#pragma once
#include <new>
#include <type_traits>

namespace kt::detail {
class erased_semantics_t {
	template <typename T>
	static constexpr bool can_copy_v = std::is_copy_constructible_v<T>&& std::is_copy_assignable_v<T>;

  public:
	template <typename T>
	struct tag_t {};

	template <typename T>
	constexpr erased_semantics_t(tag_t<T>) noexcept;

	void move_construct(void* src, void* dst) const noexcept;
	void move_assign(void* src, void* dst) const noexcept;
	void copy_construct(void const* src, void* dst) const;
	void copy_assign(void const* src, void* dst) const;
	void destroy(void const* src) const noexcept;

  private:
	template <typename T>
	static void move_c(void* src, void* dst) noexcept;
	template <typename T>
	static void move_a(void* src, void* dst) noexcept;
	template <typename T>
	static void copy_c(void const* src, void* dst);
	template <typename T>
	static void copy_a(void const* src, void* dst);

	template <typename T>
	static void destroy_(void const* pPtr) noexcept;

	using move_t = void (*)(void* src, void* dst);
	using copy_t = void (*)(void const* src, void* dst);
	using destroy_t = void (*)(void const* src);

	move_t const m_mc;
	move_t const m_ma;
	copy_t const m_cc;
	copy_t const m_ca;
	destroy_t const m_d;
};

// impl

template <typename T>
constexpr erased_semantics_t::erased_semantics_t(tag_t<T>) noexcept
	: m_mc(&move_c<T>), m_ma(&move_a<T>), m_cc(&copy_c<T>), m_ca(&copy_a<T>), m_d(&destroy_<T>) {
	static_assert(can_copy_v<T>, "T must be copiable");
}

inline void erased_semantics_t::move_construct(void* src, void* dst) const noexcept { m_mc(src, dst); }
inline void erased_semantics_t::move_assign(void* src, void* dst) const noexcept { m_ma(src, dst); }
inline void erased_semantics_t::copy_construct(void const* src, void* dst) const { m_cc(src, dst); }
inline void erased_semantics_t::copy_assign(void const* src, void* dst) const { m_ca(src, dst); }
inline void erased_semantics_t::destroy(void const* src) const noexcept { m_d(src); }
template <typename T>
void erased_semantics_t::move_c(void* src, void* dst) noexcept {
	new (dst) T(std::move(*std::launder(reinterpret_cast<T*>(src))));
}
template <typename T>
void erased_semantics_t::move_a(void* src, void* dst) noexcept {
	T* t = std::launder(reinterpret_cast<T*>(dst));
	*t = std::move(*std::launder(reinterpret_cast<T*>(src)));
}
template <typename T>
void erased_semantics_t::copy_c(void const* src, void* dst) {
	new (dst) T(*std::launder(reinterpret_cast<T const*>(src)));
}
template <typename T>
void erased_semantics_t::copy_a(void const* src, void* dst) {
	T* t = std::launder(reinterpret_cast<T*>(dst));
	*t = *std::launder(reinterpret_cast<T const*>(src));
}
template <typename T>
void erased_semantics_t::destroy_(void const* src) noexcept {
	T const* t = std::launder(reinterpret_cast<T const*>(src));
	t->~T();
}
} // namespace kt::detail
