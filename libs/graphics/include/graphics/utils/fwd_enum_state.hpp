#pragma once
#include <core/ensure.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>

namespace le::graphics {
template <typename En, std::size_t N = (std::size_t)En::eCOUNT_>
struct FwdEnumState {
	struct State {
		kt::fixed_vector<En, N> to;
		En value{};

		constexpr auto operator<=>(State const& rhs) const noexcept { return value <=> rhs.value; }
	};

	using States = kt::fixed_vector<State, N>;

	States* states{};
	State* curr{};
	State* prev{};

	State const& current() const {
		ENSURE(curr, "Null");
		return *curr;
	}

	State* find(En e) const {
		if (curr && curr->value == e) { return curr; }
		ENSURE(states, "Null");
		for (State& s : *states) {
			if (s.value == e) { return &s; }
		}
		return nullptr;
	}

	bool set(En en) noexcept {
		if (auto s = find(en)) {
			prev = curr;
			curr = s;
			return true;
		}
		return false;
	}

	En value() const { return current().value; }

	bool transition(En to, bool force) {
		for (En const e : current().to) {
			if (e == to) { return set(to); }
		}
		if (force) { set(to); }
		return false;
	}
};
} // namespace le::graphics
