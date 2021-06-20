#pragma once
#include <functional>
#include <string>
#include <core/span.hpp>

namespace le::utils {
struct Exec {
	std::string label;
	std::function<void(Span<std::string_view const>)> callback;
};
} // namespace le::utils
