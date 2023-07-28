#pragma once
#include <stdexcept>

namespace spaced {
struct Error : std::runtime_error {
	using std::runtime_error::runtime_error;
};
} // namespace spaced
