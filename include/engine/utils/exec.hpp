#pragma once
#include <core/span.hpp>
#include <ktl/async/kfunction.hpp>
#include <string>

namespace le::utils {
struct Exec {
	std::string label;
	ktl::kfunction<void(Span<std::string_view const>)> callback;
};
} // namespace le::utils
