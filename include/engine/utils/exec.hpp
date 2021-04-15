#pragma once
#include <functional>
#include <string>
#include <core/span.hpp>

namespace le::utils {
struct Exec {
	std::string label;
	std::function<void(View<std::string_view>)> callback;
};
} // namespace le::utils
