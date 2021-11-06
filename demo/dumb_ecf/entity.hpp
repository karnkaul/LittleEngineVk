#pragma once
#include <cstdint>
#include <tuple>

namespace decf {
struct entity {
	std::size_t id{};
	std::size_t registry_id{};

	constexpr bool operator==(entity const&) const = default;
	constexpr operator std::size_t() const noexcept { return id; }

	struct hasher {
		std::size_t operator()(entity const& e) const noexcept { return e.id ^ (e.registry_id << 8); }
	};
};

template <typename... Types>
using view_t = std::tuple<Types&...>;
} // namespace decf
