#include <thread>
#include <core/os.hpp>
#include <core/services.hpp>
#include <engine/engine.hpp>
#include <engine/utils/sys_info.hpp>

namespace le::utils {
SysInfo SysInfo::make() {
	SysInfo ret;
	ret.cpuID = os::cpuID();
	ret.threadCount = std::thread::hardware_concurrency();
	if (auto eng = Services::locate<Engine>(false)) {
		if (eng->booted()) { ret.gpuName = eng->gfx().boot.device.physicalDevice().name(); }
		ret.displayCount = eng->windowManager().displayCount();
	}
	return ret;
}
} // namespace le::utils
