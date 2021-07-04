#pragma once
#include <engine/input/space.hpp>
#include <engine/input/state.hpp>

namespace le::input {
struct Frame {
	State state;
	Space space;
};
} // namespace le::input
