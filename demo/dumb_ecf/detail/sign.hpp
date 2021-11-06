#pragma once
#include <cstdint>
#include <span>
#include <typeinfo>

namespace decf::detail {
struct sign_t {
	std::size_t hash{};

	bool operator==(sign_t const& rhs) const = default;
	operator std::size_t() const noexcept { return hash; }

	void add_type(sign_t rhs) noexcept { hash ^= rhs.hash; }

	struct hasher {
		std::size_t operator()(sign_t const& s) const noexcept { return s.hash; }
	};

	template <typename T>
	static sign_t make() noexcept {
		static std::size_t const ret = typeid(T).hash_code();
		return {ret};
	}

	static sign_t combine(std::span<sign_t const> types) noexcept {
		sign_t ret{};
		for (auto const sign : types) { ret.hash ^= sign.hash; }
		return ret;
	}
};
} // namespace decf::detail
