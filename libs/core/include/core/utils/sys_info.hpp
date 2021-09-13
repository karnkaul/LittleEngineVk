#pragma once
#include <string_view>
#include <thread>
#include <core/os.hpp>
#include <core/std_types.hpp>

namespace le::utils {
struct SysInfo {
	std::string gpuName = "(unknown)";
	std::string_view presentMode = "(unknown)";
	std::string_view cpuID = os::cpuID();
	u32 threadCount = std::thread::hardware_concurrency();
	std::size_t displayCount = 0;
	u32 swapchainImages = 0;
};
} // namespace le::utils
