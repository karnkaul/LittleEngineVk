#include <core/log_channel.hpp>
#include <levk/graphics/common.hpp>
#include <levk/graphics/context/bootstrap.hpp>

namespace le::graphics {
Bootstrap::Bootstrap(CreateInfo const& info, Device::MakeSurface&& makeSurface) : device(info.device, std::move(makeSurface)), vram(&device, info.transfer) {
	logI(LC_LibUser, "[{}] Vulkan bootstrapped [{}] [{}]", g_name, levk_OS_name, levk_arch_name);
}

Bootstrap::~Bootstrap() { vram.shutdown(); /* stop transfer/poll before member destruction chain */ }
} // namespace le::graphics
