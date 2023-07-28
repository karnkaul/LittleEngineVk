#pragma once
#include <concepts>
#include <iterator>

namespace spaced {
///
/// \brief Wrapper for Enumerator value types.
/// \param Type The actual value type of the source container
/// \param Index Type of the index
///
template <typename Type, typename Index>
struct Enumerated {
	Type t;
	Index index;

	constexpr auto get() const -> Type { return t; }
};

///
/// \brief "Container" around a pair of iterators along with an incrementing index
/// \param Index Type of the index
/// \param It Type of the iterators
///
template <typename Index, typename It>
class Enumerator {
  public:
	using iterator_type = It;
	using value_type = decltype(*std::declval<It>());

	struct iterator {
		using value_type = Enumerated<Enumerator::value_type, Index>;
		using difference_type = void;
		using reference = value_type;

		It it{};
		Index index{};

		auto operator*() const -> reference { return {*it, index}; }
		auto operator++() -> iterator& { return (++it, ++index, *this); }

		auto operator==(iterator const&) const -> bool = default;
	};

	using const_iterator = iterator;

	constexpr Enumerator(It first, It last) : m_begin(first), m_end(last), m_size(static_cast<std::size_t>(std::distance(first, last))) {}

	[[nodiscard]] constexpr auto begin() const -> iterator { return {m_begin, static_cast<Index>(0)}; }
	[[nodiscard]] constexpr auto end() const -> iterator { return {m_end, static_cast<Index>(m_size)}; }

  private:
	It m_begin;
	It m_end;
	std::size_t m_size;
};

///
/// \brief Enumerate a tuple of the iterators' value type and an auto-incrementing index.
/// \param Index Type of index to use
/// \param It Type of iterator
/// \param first Begin of range to iterate over
/// \param last End of range to iterate over
///
template <typename Index = std::size_t, typename It>
constexpr auto enumerate(It first, It last) {
	return Enumerator<Index, It>{first, last};
}

///
/// \brief Enumerate a tuple of the container's value type and an auto-incrementing index.
/// \param Index Type of index to use
/// \param It Type of iterator
/// \param first Begin of range to iterate over
/// \param last End of range to iterate over
///
template <typename Index = std::size_t, typename Container>
constexpr auto enumerate(Container&& container) {
	return enumerate<Index>(std::begin(container), std::end(container));
}

// tests

static_assert(std::same_as<typename Enumerator<int, int*>::value_type, int&>);
static_assert(std::same_as<typename Enumerator<int, int const*>::value_type, int const&>);
} // namespace spaced
