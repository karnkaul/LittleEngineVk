#pragma once
#include <algorithm>
#include <deque>
#include <core/tagged_store.hpp>

namespace le {
///
/// \brief std::deque-based storage for TaggedStore
///
template <typename Ty, typename Id>
struct TaggedDeque {
	using type = Ty;
	using id_t = Id;
	using container_t = std::deque<std::pair<id_t, type>>;

	container_t storage;

	template <typename... U>
	void push_back(id_t id, U&&... u) {
		storage.push_back({id, std::forward<U>(u)...});
	}
	template <typename... U>
	void push_front(id_t id, U&&... u) {
		storage.push_front({id, std::forward<U>(u)...});
	}
	void pop(id_t id) noexcept {
		auto const it = std::find_if(storage.begin(), storage.end(), [id](auto const& t) { return t.first == id; });
		storage.erase(it);
	}
	type* find(id_t id) noexcept {
		auto const it = std::find_if(storage.begin(), storage.end(), [id](auto const& t) { return t.first == id; });
		return it != storage.end() ? &it->second : nullptr;
	}
	type const* find(id_t id) const noexcept {
		auto const it = std::find_if(storage.begin(), storage.end(), [id](auto const& t) { return t.first == id; });
		return it != storage.end() ? &it->second : nullptr;
	}
};
} // namespace le
