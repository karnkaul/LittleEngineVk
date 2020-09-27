#pragma once
#include <vector>
#include <core/std_types.hpp>
#include <core/zero.hpp>

namespace le::ecs
{
using ID = TZero<u64>;

///
/// \brief Entity is a glorified, type-safe combination of its ID and its registryID
///
struct Entity final
{
	ID id;
	ID regID;
};

///
/// \brief List of `T...` components
///
template <typename... T>
using Components = std::tuple<T...>;

///
/// \brief Return type wrapper for `Registry::spawn<T...>()`
///
template <typename... T>
struct Spawned final
{
	using type = Spawned<T...>;

	Entity entity;
	Components<T&...> components;

	constexpr operator Entity() const noexcept;
};
///
/// \brief Specialisation for zero components
///
template <>
struct Spawned<>
{
	using type = Entity;
};
template <typename... T>
using Spawned_t = typename Spawned<T...>::type;

///
/// \brief Return type wrapper for Registry::view()
///
template <typename... T>
using View_t = std::vector<Spawned_t<T...>>;

///
/// \brief Hash signature of component types
///
using Sign = std::size_t;

inline constexpr bool operator==(Entity lhs, Entity rhs) noexcept
{
	return lhs.id == rhs.id && lhs.regID == rhs.regID;
}

inline constexpr bool operator!=(Entity lhs, Entity rhs) noexcept
{
	return !(lhs == rhs);
}
} // namespace le::ecs

namespace std
{
using namespace le::ecs;

template <>
struct hash<Entity>
{
	size_t operator()(Entity const& entity) const
	{
		return std::hash<ID::type>()(entity.id) ^ std::hash<ID::type>()(entity.regID);
	}
};
} // namespace std
