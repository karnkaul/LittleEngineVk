#pragma once
#include <levk/engine/input/space.hpp>
#include <levk/engine/input/state.hpp>

namespace le::input {
struct Frame {
	State state;
	Space space;
};
} // namespace le::input
