#pragma once
#include <unordered_map>
#include <core/ec/types.hpp>
#include <core/ensure.hpp>

namespace le::ecs::detail {
struct Concept {
	Sign sign = 0;

	virtual ~Concept() = default;

	virtual bool detach(Entity entity) = 0;
	virtual std::vector<Entity> entities() const noexcept = 0;
	virtual bool exists(Entity) const noexcept = 0;
	virtual std::size_t size() const noexcept = 0;
};

template <typename T>
struct Storage final : Concept {
	static_assert(std::is_move_constructible_v<T>, "T must be move constructible!");

	std::unordered_map<Entity, T> map;

	template <typename... Args>
	T& attach(Entity entity, Args&&... args);
	bool detach(Entity entity) override;
	T* find(Entity entity);
	T const* find(Entity entity) const;
	std::size_t clear();

	std::vector<Entity> entities() const noexcept override;
	bool exists(Entity entity) const noexcept override;
	std::size_t size() const noexcept override;
};

template <typename T>
template <typename... Args>
T& Storage<T>::attach(Entity entity, Args&&... args) {
	if (auto search = map.find(entity); search != map.end()) {
		ENSURE(false, "Duplicate!");
		T& ret = search->second;
		ret = T{std::forward<Args>(args)...};
		return ret;
	}
	auto [ret, bResult] = map.emplace(entity, T{std::forward<Args>(args)...});
	ENSURE(bResult, "Insertion failure!");
	return ret->second;
}

template <typename T>
bool Storage<T>::detach(Entity entity) {
	if (auto search = map.find(entity); search != map.end()) {
		map.erase(search);
		return true;
	}
	return false;
}

template <typename T>
T* Storage<T>::find(Entity entity) {
	if (auto search = map.find(entity); search != map.end()) {
		return std::addressof(search->second);
	}
	return nullptr;
}

template <typename T>
T const* Storage<T>::find(Entity entity) const {
	if (auto search = map.find(entity); search != map.end()) {
		return std::addressof(search->second);
	}
	return nullptr;
}

template <typename T>
std::size_t Storage<T>::clear() {
	auto const ret = map.size();
	map.clear();
	return ret;
}

template <typename T>
std::vector<Entity> Storage<T>::entities() const noexcept {
	std::vector<Entity> ret;
	ret.reserve(map.size());
	for (auto const& [e, _] : map) {
		ret.push_back(e);
	}
	return ret;
}

template <typename T>
bool Storage<T>::exists(Entity entity) const noexcept {
	return map.find(entity) != map.end();
}

template <typename T>
std::size_t Storage<T>::size() const noexcept {
	return map.size();
}
} // namespace le::ecs::detail
