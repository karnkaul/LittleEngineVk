#pragma once
#include <stdexcept>

namespace le {
struct Error : std::runtime_error {
	using std::runtime_error::runtime_error;
};
} // namespace le
