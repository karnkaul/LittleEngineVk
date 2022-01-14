#pragma once
#include <type_traits>

namespace le {
///
/// \brief Ultra-light reference wrapper
///
template <typename T>
	requires(!std::is_reference_v<T>)
class Ref {
  public:
	using type = T;

	constexpr Ref(T& t) noexcept : m_ptr(&t) {}

	constexpr operator T&() const noexcept { return *m_ptr; }
	constexpr T& get() const noexcept { return *m_ptr; }
	constexpr bool operator==(Ref<T> const& rhs) const { return get() == rhs.get(); }

  private:
	T* m_ptr;
};
} // namespace le
