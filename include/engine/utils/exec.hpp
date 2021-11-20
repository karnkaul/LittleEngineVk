#pragma once
#include <core/span.hpp>
#include <ktl/move_only_function.hpp>
#include <string>

namespace le::utils {
struct Exec {
	std::string label;
	ktl::move_only_function<void(Span<std::string_view const>)> callback;
};
} // namespace le::utils
