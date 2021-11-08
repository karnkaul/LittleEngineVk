#pragma once
#include <core/span.hpp>
#include <functional>
#include <string>

namespace le::utils {
struct Exec {
	std::string label;
	std::function<void(Span<std::string_view const>)> callback;
};
} // namespace le::utils
