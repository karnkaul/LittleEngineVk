#pragma once
#include <stdexcept>
#include <type_traits>
#include <typeinfo>

namespace le {
struct ErasedRef final {
	template <typename T>
	struct tag {};

	constexpr ErasedRef() = default;

	template <typename T, typename = std::enable_if_t<!std::is_same_v<T, ErasedRef>>>
	constexpr ErasedRef(T& out_t, tag<T> = tag<T>()) noexcept;

	template <typename T>
	constexpr bool contains() const noexcept;

	template <typename T>
	constexpr T& get() const;

  private:
	void* pPtr = nullptr;
	std::size_t hash = 0;
};

// impl

template <typename T, typename>
constexpr ErasedRef::ErasedRef(T& out_t, tag<T>) noexcept : pPtr(&out_t), hash(typeid(T).hash_code()) {}
template <typename T>
constexpr bool ErasedRef::contains() const noexcept {
	return hash && typeid(T).hash_code();
}
template <typename T>
constexpr T& ErasedRef::get() const {
	if (!contains<T>()) {
		throw std::runtime_error("Invalid type");
	}
	return *reinterpret_cast<T*>(pPtr);
}
} // namespace le
