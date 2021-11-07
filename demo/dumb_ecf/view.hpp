#pragma once
#include <vector>
#include <dumb_ecf/detail/archetype.hpp>

namespace decf {
class registry2;

template <typename... Types>
struct exclude {
	inline static detail::sign_t const signs[sizeof...(Types)] = {detail::sign_t::make<Types>()...};
};

template <>
struct exclude<> {
	static constexpr std::span<detail::sign_t const> signs = {};
};

template <typename... Types>
class array_view {
  public:
	array_view() = default;

	std::size_t size() const noexcept { return m_size; }
	slice_t<Types...> operator[](std::size_t index) const noexcept;

  private:
	array_view(detail::archetype const* archetype) noexcept : m_archetype(archetype), m_size(m_archetype->size()) {}
	detail::archetype const* m_archetype{};
	std::size_t m_size{};

	friend class registry2;
};

// impl

template <typename... Types>
slice_t<Types...> array_view<Types...>::operator[](std::size_t index) const noexcept {
	assert(m_archetype && index < m_archetype->size());
	return m_archetype->at<Types...>(index);
}
} // namespace decf
