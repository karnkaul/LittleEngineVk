#pragma once
#include <cstdint>
#include <tuple>

namespace dens {
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
struct entity_view {
	std::tuple<Types&...> components;
	entity entity;

	operator struct entity() const noexcept { return entity; }

	template <typename T>
	T& get() const noexcept {
		return std::get<T&>(components);
	}
};
} // namespace dens
