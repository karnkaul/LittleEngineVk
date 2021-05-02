#include <iostream>
#include <sstream>
#include <core/io/reader.hpp>
#include <core/os.hpp>
#include <engine/config.hpp>
#include <engine/engine.hpp>
#include <engine/utils/command_line.hpp>
#include <engine/utils/env.hpp>

namespace le::utils {
bool Env::init(int argc, char* const argv[], ExecMap execs) {
	os::args({argc, argv});
	bool boot = true;
	if constexpr (levk_desktopOS) {
		{
			Exec exec;
			exec.label = "list available GPUs";
			exec.callback = [&boot](View<std::string_view>) {
				std::stringstream str;
				str << "Available GPUs:\n";
				int i = 0;
				for (auto const& d : Engine::availableDevices()) {
					str << i++ << ". " << d << "\n";
				}
				std::cout << str.str();
				boot = false;
			};
			execs[{"gpu-list"}] = std::move(exec);
		}
		{
			Exec exec;
			exec.label = "device override [index]";
			exec.callback = [](View<std::string_view> args) {
				if (!args.empty()) {
					if (s64 const i = toS64(args[0], -1); i >= 0) {
						auto const idx = std::size_t(i);
						std::size_t const total = Engine::availableDevices().size();
						if (idx < total) {
							Engine::s_options.gpuOverride = idx;
							std::cout << "GPU Override set to: " << idx << '\n';
						} else {
							std::cout << "Invalid GPU Override: " << idx << "; total: " << total << '\n';
						}
					} else {
						Engine::s_options.gpuOverride.reset();
						std::cout << "GPU Override cleared\n";
					}
				}
			};
			execs[{"override-gpu", true}] = std::move(exec);
		}
		{
			Exec exec;
			exec.label = "enable VSYNC (if available)";
			exec.callback = [](View<std::string_view>) { graphics::Swapchain::s_forceVsync = true; };
			execs[{"vsync"}] = std::move(exec);
		}
		{
			Exec exec;
			exec.label = "force validation layers on/off [\"true\"|\"false\"]";
			exec.callback = [](View<std::string_view> args) {
				if (!args.empty()) {
					graphics::Instance::s_forceValidation = toBool(args[0], true);
					std::cout << "Validation layers overriden: " << (*graphics::Instance::s_forceValidation ? "on" : "off") << '\n';
				} else {
					graphics::Instance::s_forceValidation.reset();
					std::cout << "Validation layers override cleared\n";
				}
			};
			execs[{"validation", true}] = std::move(exec);
		}
	}
	CommandLine cl(std::move(execs));
	s_unused = cl.parse(os::args());
	return cl.execute(s_unused, &boot);
}

kt::result<io::Path, std::string> Env::findData(io::Path pattern, u8 maxHeight) {
	auto const root = os::dirPath(os::Dir::eExecutable);
	auto data = io::FileReader::findUpwards(root, pattern, maxHeight);
	if (!data) {
		return fmt::format("[{}] {} not found (searched {} levels up from {})", conf::g_name, pattern.generic_string(), maxHeight, root.generic_string());
	}
	return data.move();
}

void Env::runUnusedArgs(ExecMap execs) {
	CommandLine cl(std::move(execs));
	cl.execute(s_unused, nullptr);
}
} // namespace le::utils
