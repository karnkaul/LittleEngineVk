#pragma once
#include <stdexcept>
#include <type_traits>
#include <typeinfo>

namespace le {
class ErasedPtr {
  public:
	constexpr ErasedPtr() = default;

	template <typename T>
		requires(std::is_pointer_v<T>)
	constexpr ErasedPtr(T t) noexcept : m_ptr(t), m_hash(typeid(T).hash_code()) {}

	template <typename T>
		requires(std::is_pointer_v<T>)
	[[nodiscard]] constexpr bool contains() const noexcept { return m_hash == typeid(T).hash_code(); }

	template <typename T>
		requires(std::is_pointer_v<T>)
	[[nodiscard]] constexpr T get() const {
		if (!contains<T>()) { throw std::runtime_error("Invalid type"); }
		return reinterpret_cast<T>(m_ptr);
	}

  private:
	void* m_ptr = nullptr;
	std::size_t m_hash = 0;
};
} // namespace le
