#pragma once
#include <core/os.hpp>
#include <core/std_types.hpp>
#include <string_view>
#include <thread>

namespace le::utils {
struct SysInfo {
	std::string gpuName = "(unknown)";
	std::string_view presentMode = "(unknown)";
	std::string_view cpuID = os::cpuID();
	std::string_view arch = levk_arch_name;
	std::string_view osName = levk_OS_name;
	u32 threadCount = std::thread::hardware_concurrency();
	std::size_t displayCount = 0;
	u32 swapchainImages = 0;
};
} // namespace le::utils
