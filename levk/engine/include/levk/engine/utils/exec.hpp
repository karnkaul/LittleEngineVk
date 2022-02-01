#pragma once
#include <ktl/async/kfunction.hpp>
#include <levk/core/span.hpp>
#include <string>

namespace le::utils {
struct Exec {
	std::string label;
	ktl::kfunction<void(Span<std::string_view const>)> callback;
};
} // namespace le::utils
