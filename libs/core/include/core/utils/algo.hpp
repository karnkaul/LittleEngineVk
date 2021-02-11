#pragma once
#include <algorithm>
#include <vector>

namespace le::utils {
template <typename T, typename Alloc, typename U>
typename std::vector<T, Alloc>::size_type erase(std::vector<T, Alloc>& out_vec, U const& u) {
	auto it = std::remove(out_vec.begin(), out_vec.end(), u);
	auto const r = std::distance(it, out_vec.end());
	out_vec.erase(it, out_vec.end());
	return static_cast<typename std::vector<T, Alloc>::size_type>(r);
}

template <typename T, typename Alloc, typename Pred>
typename std::vector<T, Alloc>::size_type erase_if(std::vector<T, Alloc>& out_vec, Pred pred) {
	auto it = std::remove_if(out_vec.begin(), out_vec.end(), pred);
	auto const r = std::distance(it, out_vec.end());
	out_vec.erase(it, out_vec.end());
	return static_cast<typename std::vector<T, Alloc>::size_type>(r);
}
} // namespace le::utils
