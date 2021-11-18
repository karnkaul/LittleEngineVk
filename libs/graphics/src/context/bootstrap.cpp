#include <graphics/common.hpp>
#include <graphics/context/bootstrap.hpp>

namespace le::graphics {
Bootstrap::Bootstrap(CreateInfo const& info, Device::MakeSurface&& makeSurface, glm::ivec2 framebufferSize)
	: device(info.device, std::move(makeSurface)), vram(&device, info.transfer), swapchain(&vram, info.swapchain, framebufferSize) {
	g_log.minVerbosity = info.verbosity;
	g_log.log(lvl::info, 1, "[{}] Vulkan bootstrapped [{}] [{}]", g_name, levk_OS_name, levk_arch_name);
}

Bootstrap::~Bootstrap() { vram.shutdown(); /* stop transfer/poll before member destruction chain */ }
} // namespace le::graphics
