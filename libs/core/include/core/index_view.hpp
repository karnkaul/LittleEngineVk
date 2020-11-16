#pragma once
#include <utility>
#include <core/ref.hpp>

namespace le {
///
/// \brief Iterator for Container<T> and index pair
///
template <typename T, bool Const>
struct IndexIter;
///
/// \brief Policy class providing typedefs used by view and iterators
///
template <typename Cont, typename I>
struct IndexViewPolicy;
///
/// \brief Iteratable view into a container and an incrementing index
///
template <typename Cont, typename I = std::size_t, typename Policy = IndexViewPolicy<Cont, I>>
struct IndexView;
///
/// \brief Iteratable const view into a container and an incrementing index
///
template <typename Cont, typename I = std::size_t, typename Policy = IndexViewPolicy<Cont, I>>
struct ConstIndexView;
template <typename Cont, typename I = std::size_t, typename Policy = IndexViewPolicy<Cont, I>>
using CIndexView = ConstIndexView<Cont, I, Policy>;

namespace detail {
template <typename T, bool B>
using consted_t = std::conditional_t<B, T const, T>;
template <typename T, bool B>
using consted_iter_t = std::conditional_t<B, typename T::const_iter_type, typename T::iter_type>; 
} // namespace detail

template <typename Cont, typename I, typename Policy>
struct IndexView {
	using container_type = typename Policy::container_type;
	using iterator = IndexIter<Policy, false>;

	container_type& c;

	IndexView(Cont& c) : c(c) {
	}

	iterator begin() {
		return iterator(c.begin(), 0);
	}
	iterator end() {
		return iterator(c.end(), c.size());
	}
};

template <typename Cont, typename I, typename Policy>
struct ConstIndexView {
	using container_type = typename Policy::container_type;
	using const_iterator = IndexIter<Policy, true>;

	container_type const& c;

	ConstIndexView(Cont& c) : c(c) {
	}

	const_iterator begin() const {
		return const_iterator(c.begin(), 0);
	}
	const_iterator end() const {
		return const_iterator(c.end(), c.size());
	}
};

template <typename T, bool Const>
struct IndexIter {
	using value_type = typename T::value_type;
	using index_type = typename T::index_type;
	using iter_type = detail::consted_iter_t<T, Const>;
	using type = std::pair<detail::consted_t<value_type, Const>&, index_type>;
	using reference = type;
	using pointer = type;

	std::pair<iter_type, index_type> iter;

	IndexIter(iter_type iter, index_type index) : iter({iter, index}) {
	}

	reference operator*() const {
		return {*iter.first, iter.second};
	}
	pointer operator->() const {
		return {*iter.first, iter.second};
	}
	IndexIter& operator++() {
		++iter.first;
		++iter.second;
		return *this;
	}
	friend bool operator==(IndexIter const& lhs, IndexIter const& rhs) {
		return lhs.iter.first == rhs.iter.first && lhs.iter.second == rhs.iter.second;
	}
	friend bool operator!=(IndexIter const& lhs, IndexIter const& rhs) {
		return !(lhs == rhs);
	}
};

template <typename Cont, typename I>
struct IndexViewPolicy {
	using container_type = Cont;
	using index_type = I;
	using value_type = typename container_type::value_type;
	using iter_type = typename container_type::iterator;
	using const_iter_type = typename container_type::const_iterator;
};
} // namespace le
