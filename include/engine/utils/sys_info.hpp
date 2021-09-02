#pragma once
#include <string_view>
#include <core/std_types.hpp>

namespace le::utils {
struct SysInfo {
	std::string_view cpuID = "(unknown)";
	std::string_view gpuName = "(unknown)";
	u32 threadCount = 0;
	std::size_t displayCount = 0;

	static SysInfo make();
};
} // namespace le::utils
