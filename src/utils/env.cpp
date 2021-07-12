#include <iostream>
#include <sstream>
#include <core/io/reader.hpp>
#include <core/os.hpp>
#include <core/span.hpp>
#include <engine/engine.hpp>
#include <engine/utils/env.hpp>
#include <engine/utils/logger.hpp>

namespace le {
env::Run env::init(int argc, char const* const argv[], Spec::cmd_map_t cmds) {
	Run ret = Run::resume;
	os::args(os::Args(argv, std::size_t(argc)));
	{
		Spec::cmd_t gpu;
		gpu.description = "List / select from available GPUs";
		gpu.args_fmt = "[index]";
		gpu.callback = [&ret](clap::interpreter::params_t const& p) {
			if (p.arguments.empty()) {
				try {
					std::stringstream str;
					str << "Available GPUs:\n";
					int i = 0;
					for (auto const& d : Engine::availableDevices()) { str << i++ << ". " << d << "\n"; }
					std::cout << str.str();
				} catch (std::runtime_error const& e) { std::cerr << "Failed to poll GPUs: " << e.what() << '\n'; }
				ret = Run::quit;
			} else {
				if (s64 const i = utils::toS64(p.arguments[0], -1); i >= 0) {
					auto const idx = std::size_t(i);
					try {
						std::size_t const total = Engine::availableDevices().size();
						if (idx < total) {
							Engine::s_options.gpuOverride = idx;
							std::cout << "GPU Override set to: " << idx << '\n';
						} else {
							std::cout << "Invalid GPU Override: " << idx << "; total: " << total << '\n';
						}
					} catch (std::runtime_error const& e) { std::cerr << "Failed to poll GPUs:" << e.what() << '\n'; }
				}
			}
		};
		cmds["gpu"] = std::move(gpu);
	}
	Spec spec;
	spec.main.version = Engine::version().toString(false);
	spec.main.exe = os::exeName();
	spec.commands = std::move(cmds);
	{
		Spec::opt_t vsync;
		vsync.id = "vsync";
		vsync.description = "Force VSYNC (immediate mode)";
		vsync.value_fmt = "[off/on/adaptive]";
		Spec::opt_t validation;
		validation.id = "validation";
		validation.description = "Force validation layers on/off";
		spec.main.options.push_back(vsync);
		spec.main.options.push_back(validation);
		spec.main.callback = [](clap::interpreter::params_t const& p) {
			if (auto val = p.opt_value("vsync")) {
				std::cout << "Requesting no VSYNC / immediate mode\n";
				if (*val == "off") {
					graphics::Swapchain::s_forceVsync = graphics::Vsync::eOff;
				} else if (*val == "on") {
					graphics::Swapchain::s_forceVsync = graphics::Vsync::eOn;
				} else if (*val == "adaptive") {
					graphics::Swapchain::s_forceVsync = graphics::Vsync::eAdaptive;
				} else if (*val == "triple" || *val == "triple-buffer") {
					graphics::Swapchain::s_forceVsync = graphics::Vsync::eTripleBuffer;
				}
			}
			if (auto val = p.opt_value("validation")) {
				bool const b = utils::toBool(*val, false);
				graphics::Instance::s_forceValidation = b;
				std::cout << "Validation layers overriden: " << (b ? "on" : "off") << '\n';
			};
		};
	}
	clap::interpreter helper;
	auto const expr = helper.parse(argc, argv);
	auto const result = helper.interpret(std::cout, std::move(spec), expr);
	if (result == Run::quit) { return result; }
	return ret;
}

kt::result<io::Path, std::string> env::findData(io::Path pattern, u8 maxHeight) {
	auto const root = os::dirPath(os::Dir::eExecutable);
	auto data = io::FileReader::findUpwards(root, pattern, maxHeight);
	if (!data) {
		return fmt::format("[{}] {} not found (searched {} levels up from {})", utils::g_name, pattern.generic_string(), maxHeight, root.generic_string());
	}
	return std::move(data).value();
}
} // namespace le
