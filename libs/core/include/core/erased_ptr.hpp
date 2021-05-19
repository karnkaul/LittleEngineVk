#pragma once
#include <stdexcept>
#include <type_traits>
#include <typeinfo>

namespace le {
template <typename T>
concept pointer_t = std::is_pointer_v<T>;

class ErasedPtr {
  public:
	constexpr ErasedPtr() = default;

	template <pointer_t T>
	constexpr ErasedPtr(T t) noexcept : m_ptr(t), m_hash(typeid(T).hash_code()) {}

	template <pointer_t T>
	[[nodiscard]] constexpr bool contains() const noexcept {
		return m_hash == typeid(T).hash_code();
	}

	template <pointer_t T>
	[[nodiscard]] constexpr T get() const {
		if (!contains<T>()) { throw std::runtime_error("Invalid type"); }
		return reinterpret_cast<T>(m_ptr);
	}

  private:
	void* m_ptr = nullptr;
	std::size_t m_hash = 0;
};
} // namespace le
