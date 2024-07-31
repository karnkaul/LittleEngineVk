#pragma once
#include <functional>

namespace levk {
/// \brief Combine hash into given seed.
///
/// Source: `boost::hash_combine`\n
// <a href="https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine">
/// https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine</a>
/// \param out_seed mutable reference to an existing seed (hash value).
/// \param hash Hash to combine.
constexpr void hash_combine(std::size_t& out_seed, std::size_t const hash) { out_seed ^= hash + 0x9e3779b9 + (out_seed << 6) + (out_seed >> 2); }

/// \brief Combine hashes of multiple objects into given seed.
/// \param out_seed Mutable reference to existing seed (hash value).
/// \param t Objects whose hashes to combine into out_seed.
template <template <typename> typename Hasher = std::hash, typename... Types>
constexpr void hash_combine(std::size_t& out_seed, Types const&... t) {
	(hash_combine(out_seed, Hasher<Types>{}(t)), ...);
}

/// \brief Make a combined hash of multiple objects.
/// \param t Objects whose hashes to combine.
/// \returns Combined hash.
template <template <typename> typename Hasher = std::hash, typename... Types>
constexpr auto make_combined_hash(Types const&... t) -> std::size_t {
	auto ret = std::size_t{};
	hash_combine<Hasher>(ret, t...);
	return ret;
}
} // namespace levk
