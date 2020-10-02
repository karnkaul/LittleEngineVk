#pragma once
#include <vector>
#include <core/std_types.hpp>
#include <core/zero.hpp>
#if defined(__GNUG__)
#include <cxxabi.h>
#endif

namespace le::ecs {
using ID = TZero<u64>;

///
/// \brief Entity is a glorified, type-safe combination of its ID and its registryID
///
struct Entity final {
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
struct Spawned final {
	using type = Spawned<T...>;

	Entity entity;
	Components<T&...> components;

	constexpr operator Entity() const noexcept;
};
///
/// \brief Specialisation for zero components
///
template <>
struct Spawned<> {
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

namespace detail {
template <bool... B>
using require = std::enable_if_t<(B && ...)>;

inline std::string demangle(std::string_view name) {
	std::string ret(name);
#if defined(__GNUG__)
	s32 status = -1;
	char* szRes = abi::__cxa_demangle(name.data(), nullptr, nullptr, &status);
	if (status == 0) {
		ret = szRes;
	}
	std::free(szRes);
#else
	static constexpr std::string_view CLASS = "class ";
	static constexpr std::string_view STRUCT = "struct ";
	auto idx = ret.find(CLASS);
	if (idx == 0) {
		ret = ret.substr(CLASS.size());
	}
	idx = ret.find(STRUCT);
	if (idx == 0) {
		ret = ret.substr(STRUCT.size());
	}
#endif
	static constexpr std::string_view prefix = "::";
	static constexpr std::string_view skip = "<";
	auto ps = ret.find(prefix);
	auto ss = ret.find(skip);
	while (ps < ret.size() && ss > ps) {
		ret = ret.substr(ps + prefix.size());
		ps = ret.find(prefix);
		ss = ret.find(skip);
	}
	return ret;
}
} // namespace detail

inline constexpr bool operator==(Entity lhs, Entity rhs) noexcept {
	return lhs.id == rhs.id && lhs.regID == rhs.regID;
}

inline constexpr bool operator!=(Entity lhs, Entity rhs) noexcept {
	return !(lhs == rhs);
}
} // namespace le::ecs

namespace std {
using namespace le::ecs;

template <>
struct hash<Entity> {
	size_t operator()(Entity const& entity) const {
		return std::hash<ID::type>()(entity.id) ^ std::hash<ID::type>()(entity.regID);
	}
};
} // namespace std
