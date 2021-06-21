#pragma once
#include <iterator>
#include <utility>

namespace le {
template <typename Cont, typename I>
class Enumerator {
  public:
	using container_t = Cont;
	using index_t = I;
	using iterator = decltype(std::begin(std::declval<Cont&>()));
	using const_iterator = decltype(std::cbegin(std::declval<Cont&>()));
	using reverse_iterator = decltype(std::rbegin(std::declval<Cont&>()));
	using difference_type = typename std::iterator_traits<iterator>::difference_type;

	template <typename It>
	class iter_t;

	constexpr explicit Enumerator(Cont& container) noexcept : m_container(container), m_size(std::distance(std::cbegin(m_container), std::cend(m_container))) {}

	constexpr iter_t<iterator> begin() const noexcept { return {std::begin(m_container), idx(0)}; }
	constexpr iter_t<iterator> end() const noexcept { return {std::end(m_container), idx(m_size)}; }
	constexpr iter_t<const_iterator> cbegin() const noexcept { return {std::cbegin(m_container), idx(0)}; }
	constexpr iter_t<const_iterator> cend() const noexcept { return {std::cend(m_container), idx(m_size)}; }
	constexpr iter_t<reverse_iterator> rbegin() const noexcept { return {std::rbegin(m_container), idx(m_size)}; }
	constexpr iter_t<reverse_iterator> rend() const noexcept { return {std::rend(m_container), idx(0)}; }

  private:
	static constexpr I idx(difference_type d) noexcept { return static_cast<I>(d); }
	Cont& m_container;
	difference_type m_size;
};

template <typename Cont, typename I = std::size_t>
Enumerator<Cont, I> enumerate(Cont& container) noexcept {
	return Enumerator<Cont, I>(container);
}

// impl

template <typename Cont, typename I>
template <typename It>
class Enumerator<Cont, I>::iter_t {
  public:
	using index_t = I;
	using iterator_t = It;
	using traits_t = std::iterator_traits<It>;
	using it_ref_t = typename traits_t::reference;

	template <typename T>
	struct RefIdx {
		T t;
		I index;
	};

	using difference_type = typename traits_t::difference_type;
	using value_type = RefIdx<it_ref_t>;
	using pointer = value_type;
	using reference = value_type;
	using iterator_category = typename traits_t::iterator_category;

	constexpr iter_t() = default;

	constexpr iter_t& operator++() noexcept {
		++m_iter;
		++m_index;
		return *this;
	}

	constexpr iter_t operator++(int) noexcept {
		auto ret = *this;
		++(*this);
		return ret;
	}

	constexpr bool operator==(iter_t const& rhs) const noexcept { return m_iter == rhs.m_iter; }
	constexpr reference operator*() const { return {*m_iter, m_index}; }

  private:
	constexpr iter_t(It iter, I index) noexcept : m_iter(iter), m_index(index) {}

	It m_iter{};
	I m_index{};

	friend class Enumerator;
};
} // namespace le
