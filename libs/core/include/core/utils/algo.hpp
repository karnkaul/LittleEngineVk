#pragma once
#include <algorithm>
#include <iterator>
#include <vector>

namespace le::utils {
template <typename T, typename Alloc, typename U>
typename std::vector<T, Alloc>::size_type erase(std::vector<T, Alloc>& out_vec, U const& u) {
	auto const it = std::remove(out_vec.begin(), out_vec.end(), u);
	auto const r = std::distance(it, out_vec.end());
	out_vec.erase(it, out_vec.end());
	return static_cast<typename std::vector<T, Alloc>::size_type>(r);
}

template <typename T, typename Alloc, typename Pred>
typename std::vector<T, Alloc>::size_type erase_if(std::vector<T, Alloc>& out_vec, Pred pred) {
	auto const it = std::remove_if(out_vec.begin(), out_vec.end(), pred);
	auto const r = std::distance(it, out_vec.end());
	out_vec.erase(it, out_vec.end());
	return static_cast<typename std::vector<T, Alloc>::size_type>(r);
}

template <typename C, typename T>
constexpr auto find(C const& c, T const& t) noexcept {
	return std::find(std::begin(c), std::end(c), t);
}

template <typename C, typename F>
constexpr auto find_if(C const& c, F&& f) noexcept {
	return std::find_if(std::begin(c), std::end(c), std::forward<F>(f));
}

template <typename In, template <typename...> typename Out, typename... Args>
constexpr void move_append(In const& in, Out<Args...>& out) {
	if constexpr (std::is_same_v<std::decay_t<Out<Args...>>, std::vector<Args...>>) {
		out.reserve(out.size() + in.size());
	}
	std::move(std::begin(in), std::end(in), std::back_inserter(out));
}

template <typename In, template <typename...> typename Out, typename... Args>
constexpr void copy_append(In const& in, Out<Args...>& out) {
	if constexpr (std::is_same_v<std::decay_t<Out<Args...>>, std::vector<Args...>>) {
		out.reserve(out.size() + in.size());
	}
	std::copy(std::begin(in), std::end(in), std::back_inserter(out));
}

template <typename C, typename K>
constexpr bool contains(C const& cont, K const& key) noexcept {
	return cont.find(key) != cont.end();
}
} // namespace le::utils
